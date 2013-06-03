/***************************************************************************
 *                                                                         *
 * Copyright (C) 2007-2012 by Johan De Taeye, frePPLe bvba                 *
 *                                                                         *
 * All information contained herein is, and remains the property of        *
 * frePPLe.                                                                *
 * You are allowed to use and modify the source code, as long as the       *
 * software is used within your company.                                   *
 * You are not allowed to distribute the software, either in the form of   *
 * source code or in the form of compiled binaries.                        *
 *                                                                         *
 ***************************************************************************/

#include "freppleinterface.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
using namespace std;


void usage()
{
  cout << "\nfrePPLe v" << FreppleVersion() << " command line application\n"
      "\nUsage:\n"
      "  frepple [options] [files | directories]\n"
      "\nThis program reads XML input data, and executes the modeling and\n"
      "planning commands included in them.\n"
      "The XML input can be provided in the following ways:\n"
      "  - Passing one or more XML files and/or directories as arguments.\n"
      "    When a directory is specified, the application will process\n"
      "    all files with the extension '.xml'.\n"
      "  - Passing one or more Python files with the extension '.py'\n"
      "    The Python commands are executed in the embedded interpreter.\n"
      "  - When passing no file or directory arguments, input will be read\n"
      "    from the standard input. XML data can be piped to the application.\n"
      "\nOptions:\n"
      "  -validate -v  Validate the XML input for correctness.\n"
      "  -check -c     Only validate the input, without executing the content.\n"
      "  -? -h -help   Show these instructions.\n"
      "\nEnvironment: The variable FREPPLE_HOME optionally points to a\n"
      "     directory where the initialization files init.xml, init.py,\n"
      "     frepple.xsd and module libraries will be searched.\n"
      "\nReturn codes: 0 when successful, non-zero in case of errors\n"
      "\nMore information on this program: http://www.frepple.com\n\n"
      << endl;
}


int main (int argc, char *argv[])
{

  // Storing the chosen options...
  bool validate = false;
  bool validate_only = false;
  bool input = false;

  try
  {
    // Analyze the command line arguments.
    for (int i = 1; i < argc; ++i)
    {
      if (argv[i][0] == '-')
      {
        // An option on the command line
        if (!strcmp(argv[i],"-validate") || !strcmp(argv[i],"-v"))
          validate = true;
        else if (!strcmp(argv[i],"-check") || !strcmp(argv[i],"-c"))
          validate_only = true;
        else
        {
          if (strcmp(argv[i],"-?")
              && strcmp(argv[i],"-h")
              && strcmp(argv[i],"-help"))
            cout << "\nError: Option '" << argv[i]
                << "' not recognized." << endl;
          usage();
          return EXIT_FAILURE;
        }
      }
      else
      {
        // A file or directory name on the command line
        if (!input)
        {
          // Initialize the library if this wasn't done before
          FreppleInitialize(argc, argv);
          input = true;
        }
        if (strlen(argv[i])>=3 && !strcmp(argv[i]+strlen(argv[i])-3,".py"))
          // Execute as Python file
          FreppleReadPythonFile(argv[i]);
        else
          // Execute as XML file
          FreppleReadXMLFile(argv[i], validate, validate_only);
      }
    }

    // When no filenames are specified, we read the standard input
    if (!input)
    {
      FreppleInitialize(argc, argv);
      FreppleReadXMLFile(NULL, validate, validate_only);
    }
  }
  catch (const exception& e)
  {
    ostringstream ch;
    ch << "Error: " << e.what();
    FreppleLog(ch.str());
    return EXIT_FAILURE;
  }
  catch (...)
  {
    FreppleLog("Error: Unknown exception type");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
