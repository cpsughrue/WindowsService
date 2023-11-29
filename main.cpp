#include <fstream>
#include <string>
#include <windows.h>

SERVICE_STATUS ServiceStatus = {};
SERVICE_STATUS_HANDLE StatusHandle = NULL;
HANDLE ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
VOID WINAPI ServiceCtrlHandler(DWORD);

char ServiceName[] = "ConnorService";

int main(int argc, char *argv[]) {
  SERVICE_TABLE_ENTRY ServiceTable[] = {
      {(LPSTR)ServiceName, (LPSERVICE_MAIN_FUNCTION)ServiceMain}, {NULL, NULL}};

  if (StartServiceCtrlDispatcher(ServiceTable) == FALSE) {
    return GetLastError();
  }

  return 0;
}

void WriteToFile(const std::string &text, const std::string &filePath) {
  std::ofstream fileStream;
  fileStream.open(filePath, std::ios::out | std::ios::app);

  if (fileStream.is_open()) {
    fileStream << text << std::endl;
    fileStream.close();
  }
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv) {
  StatusHandle = RegisterServiceCtrlHandler(ServiceName, ServiceCtrlHandler);
  if (StatusHandle == NULL) {
    return;
  }

  // Initialize Service Status
  ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
  ServiceStatus.dwControlsAccepted = 0;
  ServiceStatus.dwWin32ExitCode = 0;
  ServiceStatus.dwServiceSpecificExitCode = 0;
  ServiceStatus.dwCheckPoint = 0;
  ServiceStatus.dwWaitHint = 0;

  if (!SetServiceStatus(StatusHandle, &ServiceStatus)) {
    // Handle error
  }

  // Create the service stop event
  ServiceStopEvent = CreateEvent(NULL,  // Default security attributes
                                 TRUE,  // Manual reset event
                                 FALSE, // Initially not signaled
                                 NULL); // No option name

  // Handle error
  if (ServiceStopEvent == NULL) {
    ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    ServiceStatus.dwWin32ExitCode = GetLastError();
    SetServiceStatus(StatusHandle, &ServiceStatus);
    return;
  }

  // Update the service status to running
  ServiceStatus.dwCurrentState = SERVICE_RUNNING;
  ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
  ServiceStatus.dwCheckPoint = 0;
  ServiceStatus.dwWaitHint = 0;

  if (!SetServiceStatus(StatusHandle, &ServiceStatus)) {
    // Handle error
  }

  while (1) {

    Sleep(2000);
    WriteToFile("Writing to service file", "C:\\Program Files\\WindowsLog\\log.txt");

    // Break if service stop event is signaled
    if (WaitForSingleObject(ServiceStopEvent, 0) == WAIT_OBJECT_0) {
      break;
    }
  }

  return;
}

VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode) {
  switch (CtrlCode) {
  case SERVICE_CONTROL_STOP:
    if (ServiceStatus.dwCurrentState != SERVICE_RUNNING)
      break;

    ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
    SetServiceStatus(StatusHandle, &ServiceStatus);

    // Signal the service to stop
    SetEvent(ServiceStopEvent);

    // Close the event handle
    if (ServiceStopEvent != NULL && ServiceStopEvent != INVALID_HANDLE_VALUE) {
      CloseHandle(ServiceStopEvent);
      ServiceStopEvent = NULL;
    }

    // Update the service status to stopped
    ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(StatusHandle, &ServiceStatus);

    return;

  default:
    break;
  }
}
