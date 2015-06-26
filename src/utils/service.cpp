/***************************************************************************
 *                                                                         *
 * Copyright (C) 2007-2012 by frePPLe bvba                                 *
 *                                                                         *
 * All information contained herein is, and remains the property of        *
 * frePPLe.                                                                *
 * You are allowed to use and modify the source code, as long as the       *
 * software is used within your company.                                   *
 * You are not allowed to distribute the software, either in the form of   *
 * source code or in the form of compiled binaries.                        *
 *                                                                         *
 ***************************************************************************/

#if defined(WIN32) && !defined(__CYGWIN__)
#define FREPPLE_CORE
#include "frepple/utils.h"

namespace frepple
{
namespace utils
{


SERVICE_STATUS_HANDLE Service::statusHandle = NULL;
SERVICE_STATUS Service::status;


int Service::install()
{
  // Get path of the executable and append command line arguments
  char szPath[MAX_PATH+11];
  if (GetModuleFileName(NULL, szPath, MAX_PATH) == 0)
  {
    logger << "Can't find executable - error " << GetLastError() << endl;
    return EXIT_FAILURE;
  }
  strcat(szPath, " -service");

  // Open the local default service control manager database
  SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, 
    SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
  if (schSCManager == NULL)
  {
    logger << "Can't connect to service manager - error " << GetLastError() << endl;
    return EXIT_FAILURE;
  } 

  // Check if it exists already 
  SC_HANDLE schService = OpenService(schSCManager, "frepple", SERVICE_QUERY_CONFIG);
  if (schService != NULL)
  {
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    logger << "frePPLe service is already installed" << endl;
    return EXIT_SUCCESS;
  }

  // Install the service into SCM by calling CreateService
  schService = CreateService(
      schSCManager,                   // SCManager database
      "frepple",                      // Name of service
      "frePPLe planning service",     // Name to display
      SERVICE_QUERY_STATUS,           // Desired access
      SERVICE_WIN32_OWN_PROCESS,      // Service type
      SERVICE_DEMAND_START,           // Service start type
      SERVICE_ERROR_NORMAL,           // Error control type
      szPath,                         // Service's binary
      NULL,                           // No load ordering group
      NULL,                           // No tag identifier
      "",                             // Dependencies
      NULL,                           // Service running account
      NULL                            // Password of the account
      );
  if (schService == NULL)
  {
      logger << "Can't create service - error " << GetLastError() << endl;
      CloseServiceHandle(schSCManager);
      return EXIT_FAILURE;
  }

  // Wrap it up
  logger << "frepple service is installed." << endl;
  CloseServiceHandle(schSCManager);
  CloseServiceHandle(schService);
  return EXIT_SUCCESS;
}


int Service::uninstall()
{
  // Open the local default service control manager database
  SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
  if (schSCManager == NULL)
  {
    logger << "Can't connect to service manager - error " << GetLastError() << endl;
    return EXIT_FAILURE;
  }

  // Open the service with delete, stop, and query status permissions
  SC_HANDLE schService = OpenService(schSCManager, "frepple", 
    SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
  if (schService == NULL)
  {
    DWORD result = GetLastError();
    if (result == ERROR_SERVICE_DOES_NOT_EXIST)
    {
      logger << "frePPLe service is not installed" << endl;
      CloseServiceHandle(schSCManager);
      return EXIT_SUCCESS;
    }
    else
    {
      logger << "Can't open the service - error " << GetLastError() << endl;
      CloseServiceHandle(schSCManager);
      return EXIT_FAILURE;
    }
  }

  // Try to stop the service
  SERVICE_STATUS ssSvcStatus = {};
  if (ControlService(schService, SERVICE_CONTROL_STOP, &ssSvcStatus))
  {
    logger << "Stopping frePPLe service.";
    Sleep(1000);

    while (QueryServiceStatus(schService, &ssSvcStatus))
    {
      if (ssSvcStatus.dwCurrentState == SERVICE_STOP_PENDING)
      {
        logger << ".";
        Sleep(1000);
      }
      else break;
    }

    if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED)
      logger << endl << "frePPLe service is stopped." << endl;
    else
      logger << endl << "frePPLe service failed to stop." << endl;
  }

  // Now remove the service by calling DeleteService.
  if (!DeleteService(schService))
  {
    logger << "Deleting frePPLe service failed - error " << GetLastError() << endl;
    CloseServiceHandle(schSCManager);
    CloseServiceHandle(schService);
    return EXIT_FAILURE;
  }

  // Wrap it up
  logger << "frePPLe service is removed." << endl;
  CloseServiceHandle(schSCManager);
  CloseServiceHandle(schService);
  return EXIT_SUCCESS;
}


int Service::run()
{
      ofstream of;
      of.open("c:\\temp\\alive.txt", ios::out | ios::app);
      of << "runner " << Date::now() << endl;
      of.close();
   SERVICE_TABLE_ENTRY serviceTable[] = 
    {
      { "frepple", ServiceMain },
      { NULL, NULL }
    };

  if (StartServiceCtrlDispatcher(serviceTable) == 0)
  {
    DWORD result = GetLastError();
    if (result == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
    {
      logger << "Don't use the -service option from the command line." << endl;
      logger << "This option is only valid when the program is started as a service." << endl;
    }
    else
      logger << "frePPLe service failed to run - error " << result << endl;
    return EXIT_FAILURE;
  }
  else
    return EXIT_SUCCESS;
}


DECLARE_EXPORT void WINAPI Service::ServiceMain(DWORD argc, LPTSTR *argv)
{
  // The service runs in its own process.
  status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  // The service is starting.
  status.dwCurrentState = SERVICE_START_PENDING; 
  // The accepted commands of the service.
  status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
  status.dwWin32ExitCode = NO_ERROR;
  status.dwServiceSpecificExitCode = 0;
  status.dwCheckPoint = 0;
  status.dwWaitHint = 0;

  // Create handler
  statusHandle = RegisterServiceCtrlHandler("frepple", CtrlHandler);
  if (statusHandle == NULL) throw GetLastError();

  // Start the service.
  try
  {
      // Tell SCM that the service is starting.
      SetStatus(SERVICE_START_PENDING);

      ofstream of;
      of.open("c:\\temp\\alive.txt", ios::out | ios::app);
      of << "active" << endl;
      of.close();

      // Perform service-specific initialization.
      logger << "Starting the service" << endl;
::Sleep(2000);
      // Tell SCM that the service is started.
      SetStatus(SERVICE_RUNNING);
::Sleep(2000);
  }
  catch (DWORD dwError)
  {
      // Log the error.
      WriteErrorLogEntry("Service Start", dwError);

      // Set the service status to be stopped.
      SetStatus(SERVICE_STOPPED, dwError);
  }
  catch (...)
  {
      // Log the error.
      WriteEventLogEntry("Service failed to start.", EVENTLOG_ERROR_TYPE);

      // Set the service status to be stopped.
      SetStatus(SERVICE_STOPPED);
  }
}


DECLARE_EXPORT void WINAPI Service::CtrlHandler(DWORD dwCtrl)
{
  // This function is called by the SCM whenever a control code is
  // sent to the service. 
  if (dwCtrl == SERVICE_CONTROL_STOP || dwCtrl == SERVICE_CONTROL_SHUTDOWN)
  {
    DWORD dwOriginalState = status.dwCurrentState;
    try
    {
      // Perform service-specific shutdown operations.
      logger << "shutdown" << endl;
      // Tell SCM that the service is stopped.
      SetStatus(SERVICE_STOPPED);
    }
    catch (DWORD dwError)
    {
      WriteErrorLogEntry("Service Shutdown", dwError);                
      if (dwCtrl == SERVICE_CONTROL_STOP)
        SetStatus(dwOriginalState);
    }
    catch (...)
    {
      WriteEventLogEntry("Service failed to shut down.", EVENTLOG_ERROR_TYPE);
      if (dwCtrl == SERVICE_CONTROL_STOP)
        SetStatus(dwOriginalState);
    }
  }
  else
    logger << "Invalid service control code" << endl;
}


void Service::SetStatus(DWORD dwCurrentState, 
   DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;
    // Fill in the SERVICE_STATUS structure of the service.
    status.dwCurrentState = dwCurrentState;
    status.dwWin32ExitCode = dwWin32ExitCode;
    status.dwWaitHint = dwWaitHint;
    status.dwCheckPoint = 
        ((dwCurrentState == SERVICE_RUNNING) ||
        (dwCurrentState == SERVICE_STOPPED)) ? 
        0 : dwCheckPoint++;

    // Report the status of the service to the SCM.
    ::SetServiceStatus(statusHandle, &status);
}


void Service::WriteEventLogEntry(const char* pszMessage, unsigned short wType)
{
  HANDLE hEventSource = NULL;
  LPCSTR lpszStrings[2] = { NULL, NULL };

  hEventSource = RegisterEventSource(NULL, "frepple");
  if (hEventSource)
  {
    lpszStrings[0] = "frepple";
    lpszStrings[1] = pszMessage;
    ReportEvent(hEventSource,  // Event log handle
        wType,                 // Event type
        0,                     // Event category
        0,                     // Event identifier
        NULL,                  // No security identifier
        2,                     // Size of lpszStrings array
        0,                     // No binary data
        lpszStrings,           // Array of strings
        NULL                   // No binary data
        );
    DeregisterEventSource(hEventSource);
  }
}


void Service::WriteErrorLogEntry(const char* pszFunction, unsigned long dwError)
{
  if (!dwError) dwError = GetLastError();
  char szMessage[260];
  printf(szMessage, 260, "%s failed w/err 0x%08lx", pszFunction, dwError);
  WriteEventLogEntry(szMessage, EVENTLOG_ERROR_TYPE);
}


} // End namespace frepple_enterprise::utils
} // End namespace frepple_enterprise
#endif
