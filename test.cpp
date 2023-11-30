#include <fstream>
#include <iostream>
#include <string>
#include <windows.h>

SERVICE_STATUS ServiceStatus = {};
SERVICE_STATUS_HANDLE StatusHandle = NULL;
HANDLE ServiceStopEvent = INVALID_HANDLE_VALUE;

char ServiceName[] = "ConnorService";

// Function to install the service
void InstallService(char *argv[]) {
  SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
  if (schSCManager == NULL) {
    std::cerr << "OpenSCManager failed: " << GetLastError() << std::endl;
    return;
  }

  std::cout << argv[0] << std::endl;

  // Check if the service already exists
  SC_HANDLE schService =
      OpenService(schSCManager, ServiceName, SERVICE_QUERY_STATUS);
  if (schService != NULL) {
    std::cerr << "Service is already installed" << std::endl;
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return;
  }

  // Create the service
  schService = CreateService(
      schSCManager,                                 // SCM database
      ServiceName,                                  // name of service
      ServiceName,                                  // service name to display
      SERVICE_ALL_ACCESS,                           // desired access
      SERVICE_WIN32_OWN_PROCESS,                    // service type
      SERVICE_DEMAND_START,                         // start type
      SERVICE_ERROR_NORMAL,                         // error control type
      "C:\\Users\\cpsug\\repos\\service\\test.exe", // path to service's binary
      NULL,                                         // no load ordering group
      NULL,                                         // no tag identifier
      NULL,                                         // no dependencies
      NULL,                                         // LocalSystem account
      NULL);                                        // no password

  if (schService == NULL) {
    std::cerr << "CreateService failed: " << GetLastError() << std::endl;
  } else {
    std::cout << "Service installed successfully" << std::endl;

    // Start the service
    if (!StartService(schService, 0, NULL)) {
      std::cerr << "StartService failed: " << GetLastError() << std::endl;
    } else {
      std::cout << "Service started successfully" << std::endl;
    }
  }

  CloseServiceHandle(schService);
  CloseServiceHandle(schSCManager);
}

// Function to stop and uninstall the service
void UninstallService() {
  SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (schSCManager == NULL) {
    std::cerr << "OpenSCManager failed: " << GetLastError() << std::endl;
    return;
  }

  SC_HANDLE schService = OpenService(schSCManager, ServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
  if (schService == NULL) {
    std::cerr << "Service does not exist" << std::endl;
    CloseServiceHandle(schSCManager);
    return;
  }

  // Check the current status of the service
  SERVICE_STATUS_PROCESS ssp;
  DWORD bytesNeeded;
  if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded)) {
    std::cerr << "QueryServiceStatusEx failed: " << GetLastError() << std::endl;
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return;
  }

  // Stop the service if it is running
  if (ssp.dwCurrentState != SERVICE_STOPPED && ssp.dwCurrentState != SERVICE_STOP_PENDING) {
    if (!ControlService(schService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp)) {
      std::cerr << "ControlService failed: " << GetLastError() << std::endl;
    } else {
      std::cout << "Stopping service..." << std::endl;
      Sleep(1000);

      while (QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded)) {
        if (ssp.dwCurrentState == SERVICE_STOPPED)
          break;

        Sleep(1000);
      }
    }
  }

  // Delete the service
  if (!DeleteService(schService)) {
    std::cerr << "DeleteService faileddd: " << GetLastError() << std::endl;
  } else {
    std::cout << "Service uninstalled successfully" << std::endl;
  }

  CloseServiceHandle(schService);
  CloseServiceHandle(schSCManager);
}

void WriteToFile(const std::string &text, const std::string &filePath) {
  std::ofstream fileStream;
  fileStream.open(filePath, std::ios::out | std::ios::app);

  if (fileStream.is_open()) {
    fileStream << text << std::endl;
    fileStream.close();
  }
};

void ServiceCtrlHandlerStopService() {

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
}

VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode) {
  SC_HANDLE schSCManager = NULL;
  SC_HANDLE schService = NULL;

  switch (CtrlCode) {
  case SERVICE_CONTROL_STOP: {
    if (ServiceStatus.dwCurrentState != SERVICE_RUNNING)
      break;
    ServiceCtrlHandlerStopService();
  }
  case SERVICE_CONTROL_SHUTDOWN: {
    ServiceCtrlHandler(SERVICE_CONTROL_STOP);

    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == NULL) {
      CloseServiceHandle(schSCManager);
      break;
    }
    SC_HANDLE schService = OpenService(schSCManager, ServiceName, DELETE);
    if (schService == NULL) {
      CloseServiceHandle(schService);
      break;
    }

    DeleteService(schService);

    CloseServiceHandle(schSCManager);
    CloseServiceHandle(schService);
    return;
  }
  default:
    break;
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


// ModuleBuildDaemonServer.forkDaemon();
int main(int argc, char *argv[]) {

  if (argc > 1) {
    std::string param(argv[1]);
    if (param == "-install") {
      InstallService(argv);
      return 0;
    } else if (param == "-uninstall") {
      UninstallService();
      return 0;
    } else {
      std::cerr << "Invalid parameter provided" << std::endl;
      return 0;
    }
  }

  // ServiceMain will create the socket and start listening for connections
  // Hopefully "ServiceMain" will be identical on unix and windows
  // On windows "ServiceMain" will have it's own execution context and will have to recreate the ModuleBuildDaemonServer
  //  this might be a good reason to run the fork before ModuleBuildDaemonServer is created. Then the creation
  //  of that ModuleBuildDaemonServer can fall inside "ServiceMain" 
  SERVICE_TABLE_ENTRY ServiceTable[] = {
      {(LPSTR)ServiceName, (LPSERVICE_MAIN_FUNCTION)ServiceMain}, {NULL, NULL}};

  if (StartServiceCtrlDispatcher(ServiceTable) == FALSE) {
    return GetLastError();
  }

  return 0;
}
