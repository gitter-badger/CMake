/*=========================================================================

  Program:   CMake - Cross-Platform Makefile Generator
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Kitware, Inc., Insight Consortium.  All rights reserved.
  See Copyright.txt or http://www.cmake.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "cmExecuteProcessCommand.h"
#include "cmSystemTools.h"

#include <cmsys/Process.h>

// cmExecuteProcessCommand
bool cmExecuteProcessCommand::InitialPass(std::vector<std::string> const& args)
{
  if(args.size() < 1 )
    {
    this->SetError("called with incorrect number of arguments");
    return false;
    }
  std::vector< std::vector<const char*> > cmds;
  std::string arguments;
  bool doing_command = false;
  unsigned int command_index = 0;
  bool output_quiet = false;
  bool error_quiet = false;
  std::string timeout_string;
  std::string input_file;
  std::string output_file;
  std::string error_file;
  std::string output_variable;
  std::string error_variable;
  std::string result_variable;
  std::string working_directory;
  for(size_t i=0; i < args.size(); ++i)
    {
    if(args[i] == "COMMAND")
      {
      doing_command = true;
      command_index = cmds.size();
      cmds.push_back(std::vector<const char*>());
      }
    else if(args[i] == "OUTPUT_VARIABLE")
      {
      doing_command = false;
      if(++i < args.size())
        {
        output_variable = args[i];
        }
      else
        {
        this->SetError(" called with no value for OUTPUT_VARIABLE.");
        return false;
        }
      }
    else if(args[i] == "ERROR_VARIABLE")
      {
      doing_command = false;
      if(++i < args.size())
        {
        error_variable = args[i];
        }
      else
        {
        this->SetError(" called with no value for ERROR_VARIABLE.");
        return false;
        }
      }
    else if(args[i] == "RESULT_VARIABLE")
      {
      doing_command = false;
      if(++i < args.size())
        {
        result_variable = args[i];
        }
      else
        {
        this->SetError(" called with no value for RESULT_VARIABLE.");
        return false;
        }
      }
    else if(args[i] == "WORKING_DIRECTORY")
      {
      doing_command = false;
      if(++i < args.size())
        {
        working_directory = args[i];
        }
      else
        {
        this->SetError(" called with no value for WORKING_DIRECTORY.");
        return false;
        }
      }
    else if(args[i] == "INPUT_FILE")
      {
      doing_command = false;
      if(++i < args.size())
        {
        input_file = args[i];
        }
      else
        {
        this->SetError(" called with no value for INPUT_FILE.");
        return false;
        }
      }
    else if(args[i] == "OUTPUT_FILE")
      {
      doing_command = false;
      if(++i < args.size())
        {
        output_file = args[i];
        }
      else
        {
        this->SetError(" called with no value for OUTPUT_FILE.");
        return false;
        }
      }
    else if(args[i] == "ERROR_FILE")
      {
      doing_command = false;
      if(++i < args.size())
        {
        error_file = args[i];
        }
      else
        {
        this->SetError(" called with no value for ERROR_FILE.");
        return false;
        }
      }
    else if(args[i] == "TIMEOUT")
      {
      doing_command = false;
      if(++i < args.size())
        {
        timeout_string = args[i];
        }
      else
        {
        this->SetError(" called with no value for TIMEOUT.");
        return false;
        }
      }
    else if(args[i] == "OUTPUT_QUIET")
      {
      doing_command = false;
      output_quiet = true;
      }
    else if(args[i] == "ERROR_QUIET")
      {
      doing_command = false;
      error_quiet = true;
      }
    else if(doing_command)
      {
      cmds[command_index].push_back(args[i].c_str());
      }
    }

  // Check for commands given.
  if(cmds.empty())
    {
    this->SetError(" called with no COMMAND argument.");
    return false;
    }
  for(unsigned int i=0; i < cmds.size(); ++i)
    {
    if(cmds[i].empty())
      {
      this->SetError(" given COMMAND argument with no value.");
      return false;
      }
    else
      {
      // Add the null terminating pointer to the command argument list.
      cmds[i].push_back(0);
      }
    }

  // Parse the timeout string.
  double timeout = -1;
  if(!timeout_string.empty())
    {
    if(sscanf(timeout_string.c_str(), "%lg", &timeout) != 1)
      {
      this->SetError(" called with TIMEOUT value that could not be parsed.");
      return false;
      }
    }

  // Create a process instance.
  cmsysProcess* cp = cmsysProcess_New();

  // Set the command sequence.
  for(unsigned int i=0; i < cmds.size(); ++i)
    {
    cmsysProcess_AddCommand(cp, &*cmds[i].begin());
    }

  // Set the process working directory.
  if(!working_directory.empty())
    {
    cmsysProcess_SetWorkingDirectory(cp, working_directory.c_str());
    }

  // Always hide the process window.
  cmsysProcess_SetOption(cp, cmsysProcess_Option_HideWindow, 1);

  // Check the output variables.
  bool merge_output = (output_variable == error_variable);
  if(error_variable.empty() && !error_quiet)
    {
    cmsysProcess_SetPipeShared(cp, cmsysProcess_Pipe_STDERR, 1);
    }
  if(!input_file.empty())
    {
    cmsysProcess_SetPipeFile(cp, cmsysProcess_Pipe_STDIN, input_file.c_str());
    }
  if(!output_file.empty())
    {
    cmsysProcess_SetPipeFile(cp, cmsysProcess_Pipe_STDOUT, output_file.c_str());
    }
  if(!error_file.empty())
    {
    cmsysProcess_SetPipeFile(cp, cmsysProcess_Pipe_STDERR, error_file.c_str());
    }

  // Set the timeout if any.
  if(timeout >= 0)
    {
    cmsysProcess_SetTimeout(cp, timeout);
    }

  // Start the process.
  cmsysProcess_Execute(cp);

  // Read the process output.
  std::vector<char> tempOutput;
  std::vector<char> tempError;
  int length;
  char* data;
  int p;
  while((p = cmsysProcess_WaitForData(cp, &data, &length, 0), p))
    {
    // Translate NULL characters in the output into valid text.
    for(int i=0; i < length; ++i)
      {
      if(data[i] == '\0')
        {
        data[i] = ' ';
        }
      }

    // Put the output in the right place.
    if(p == cmsysProcess_Pipe_STDOUT && !output_quiet ||
       p == cmsysProcess_Pipe_STDERR && !error_quiet && merge_output)
      {
      if(output_variable.empty())
        {
        cmSystemTools::Stdout(data, length);
        }
      else
        {
        tempOutput.insert(tempOutput.end(), data, data+length);
        }
      }
    else if(p == cmsysProcess_Pipe_STDERR && !error_quiet)
      {
      if(!error_variable.empty())
        {
        tempError.insert(tempError.end(), data, data+length);
        }
      }
    }

  // All output has been read.  Wait for the process to exit.
  cmsysProcess_WaitForExit(cp, 0);

  // Store the output obtained.
  if(!output_variable.empty())
    {
    tempOutput.push_back('\0');
    m_Makefile->AddDefinition(output_variable.c_str(), &*tempOutput.begin());
    }
  if(!merge_output && !error_variable.empty())
    {
    tempError.push_back('\0');
    m_Makefile->AddDefinition(error_variable.c_str(), &*tempError.begin());
    }

  // Store the result of running the process.
  if(!result_variable.empty())
    {
    switch(cmsysProcess_GetState(cp))
      {
      case cmsysProcess_State_Exited:
        {
        int v = cmsysProcess_GetExitValue(cp);
        char buf[100];
        sprintf(buf, "%d", v);
        m_Makefile->AddDefinition(result_variable.c_str(), buf);
        }
        break;
      case cmsysProcess_State_Exception:
        m_Makefile->AddDefinition(result_variable.c_str(),
                                  cmsysProcess_GetExceptionString(cp));
        break;
      case cmsysProcess_State_Error:
        m_Makefile->AddDefinition(result_variable.c_str(),
                                  cmsysProcess_GetErrorString(cp));
        break;
      case cmsysProcess_State_Expired:
        m_Makefile->AddDefinition(result_variable.c_str(),
                                  "Process terminated due to timeout");
        break;
      }
    }

  // Delete the process instance.
  cmsysProcess_Delete(cp);

  return true;
}
