/*
    This file was written by Loris Degioanni, and is part of Kismet

    Kismet is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Kismet is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Kismet; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "config.h"

#ifdef SYS_CYGWIN

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define HANDLE2FD_INTERNAL
#include "handle2fd.h"

Handle2Fd::Handle2Fd() {
    NHandles = 1;
    ThreadAlive = 1;
    PipeSignalled = 0;
    WaitThreadHandle = NULL;
    FirstFdSet = 1;
    InitializeCriticalSection(&PipeCs);
}

Handle2Fd::~Handle2Fd() {
    // Kill the thread and wait until he's returned
    ThreadAlive = 0;
    SetEvent(WinHandles[0]);
    WaitForSingleObject(WaitThreadHandle, INFINITE);
}

// Set the pipe fd so that it unblocks select
void Handle2Fd::SetPipe() {
    int val;

    EnterCriticalSection(&PipeCs);

    if (!PipeSignalled) {
        write(PipeFds[1], &val, sizeof(val));
        fdatasync(PipeFds[1]);
        PipeSignalled = 1;
    }

   LeaveCriticalSection(&PipeCs);
}

// Reset the pipe fd so that it blocks select
void Handle2Fd::ResetPipe()
{
    int val;

    EnterCriticalSection(&PipeCs);

    // First, write something to be sure the read will not block
    write(PipeFds[1], &val, sizeof(val));
    fdatasync(PipeFds[1]);

    // Second, we drain the pipe
    while(read(PipeFds[0], ResetBuf, sizeof(ResetBuf)) == sizeof(ResetBuf));

    // Third, we clear the signalled flag
    PipeSignalled = 0;

    LeaveCriticalSection(&PipeCs);
}

// This thread handles asynchronously waiting on the Windows events.
// It signals the pipe if one or more events are set.
DWORD WINAPI Handle2Fd::WaitThread(LPVOID lpParameter)
{
    DWORD WaitRes;
    Handle2Fd* This = (Handle2Fd*)lpParameter;

    while (This->ThreadAlive) {

        WaitRes = WaitForMultipleObjects(This->NHandles,
            This->WinHandles,
            FALSE,
            INFINITE);

        if (WaitRes != WAIT_TIMEOUT) {
            if (WaitRes != WAIT_OBJECT_0) {		// Event 0 is the service event used to kill the thread
                This->SetPipe();
            }
        }
    }

    return 1;
}

// Activate this instance of the Handle2Fd class.
// This involves creating the pipe, the service event and the support thread
int Handle2Fd::Activate() {

    // Create the pipe
    if (pipe(PipeFds) != 0) {
        return -1;
    }

    // The fd stars in non-signaled state
    ResetPipe();

    // Create the event for pipe control, and put it in our list
    PipeControlEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!PipeControlEvent) {
        close(PipeFds[0]);
        close(PipeFds[1]);
        return -1;
    }

    WinHandles[0] = PipeControlEvent; 

    // Start the thread that does the handle checking
    if ((WaitThreadHandle = CreateThread(
        NULL,
        0,
        Handle2Fd::WaitThread,
        this,
        0,
        NULL)) == NULL) {
            close(PipeFds[0]);
            close(PipeFds[1]);
            CloseHandle(PipeControlEvent);
            return -1;
		}

    return 1;
}

// The pipe exported by the Handle2Fd class requires manual reset.
void Handle2Fd::Reset() {
        ResetPipe();
}

// Add a new handle to the class
int Handle2Fd::AddHandle(HANDLE h) {
    // If the thread is running, we don't accept new handles. This reduces the syncronization requirements
    if (!WaitThreadHandle) {
        if (NHandles < sizeof(WinHandles) / sizeof(WinHandles[0]) - 1) {
            WinHandles[NHandles++] = h;
            return 1;
        }
    }

    return -1;
}

// Get the pipe file descriptor.
int Handle2Fd::GetFd() {
    return PipeFds[0];
}

// Kismet-like MergeSet function
unsigned int Handle2Fd::MergeSet(fd_set *set, unsigned int max) {
    Reset();	// Manual reset

    if (!FD_ISSET(GetFd(), set)) {
        FD_SET(PipeFds[0], set);
        if (FirstFdSet) {
            max++;
            FirstFdSet = 0;
        }
    }

    return max;
}

// Nonzero if the HandleNumber event is set
int Handle2Fd::IsEventSet(unsigned int HandleNumber) {
    if (WaitForSingleObject(WinHandles[HandleNumber + 1], 0) == WAIT_OBJECT_0) {
        return 1;
    }
    else {
        return 0;
    }
}

#endif

