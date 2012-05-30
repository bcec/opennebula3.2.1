# -------------------------------------------------------------------------- #
# Copyright 2002-2012, OpenNebula Project Leads (OpenNebula.org)             #
#                                                                            #
# Licensed under the Apache License, Version 2.0 (the "License"); you may    #
# not use this file except in compliance with the License. You may obtain    #
# a copy of the License at                                                   #
#                                                                            #
# http://www.apache.org/licenses/LICENSE-2.0                                 #
#                                                                            #
# Unless required by applicable law or agreed to in writing, software        #
# distributed under the License is distributed on an "AS IS" BASIS,          #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   #
# See the License for the specific language governing permissions and        #
# limitations under the License.                                             #
#--------------------------------------------------------------------------- #

require "DriverExecHelper"

# This module provides an abstraction to generate an execution context for
# OpenNebula Drivers

class VirtualNetworkDriver
    include DriverExecHelper

    # Inits the VNET Driver
    # @param [String] name of the vnet driver to use, as listed in remotes/vnet
    # @option ops [String] :ssh_stream to be used for command execution
    # @option ops [String] :message from ONE
    def initialize(directory, options={})

        @options    = options
        @ssh_stream = options[:ssh_stream]
        @message    = options[:message]

        @vm_encoded = Base64.encode64(@message.elements['VM'].to_s).delete("\n")

        initialize_helper("vnm/#{directory}", options)
    end

    # Calls remotes or local action checking the action name and
    # @local_actions. Optional arguments can be specified as a hash
    #
    # @param [Number, String] id action identifier
    # @param [String, Symbol] aname name of the action
    # @param [Hash] ops extra options for the command
    # @option ops [String] :stdin text to be writen to stdin
    # @option ops [String] :parameters additional parameters for vnm action
    def do_action(id, aname, ops = {})
        options={
            :stdin      => nil,
            :parameters => nil
        }.merge(ops)

        cmd_params =  "#{@vm_encoded}"
        cmd_params << " #{options[:parameters]}" if options[:parameters]

        cmd = action_command_line(aname, cmd_params)

        if action_is_local?(aname)
            execution = LocalCommand.run(cmd, log_method(id))
        elsif @ssh_stream != nil
            if options[:stdin]
                cmdin = "cat << EOT | #{cmd}"
                stdin = "#{options[:stdin]}\nEOT\n"
            else
                cmdin = cmd
                stdin = nil
            end

            execution = @ssh_stream.run(cmdin, stdin, cmd)
        else
            return RESULT[:failure], "Network action #{aname} needs a ssh stream."
        end

        return get_info_from_execution(execution)
    end
end
