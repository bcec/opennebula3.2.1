#!/usr/bin/env ruby

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

ONE_LOCATION=ENV["ONE_LOCATION"]

if !ONE_LOCATION
    ETC_LOCATION="/etc/one"
    LIB_LOCATION="/usr/lib/one"
    RUBY_LIB_LOCATION="/usr/lib/one/ruby"
    VAR_LOCATION="/var/lib/one"
else
    ETC_LOCATION=ONE_LOCATION+"/etc"
    LIB_LOCATION=ONE_LOCATION+"/lib"
    RUBY_LIB_LOCATION=ONE_LOCATION+"/lib/ruby"
    VAR_LOCATION=ONE_LOCATION+"/var"
end

$: << LIB_LOCATION + "/sunstone/models"
$: << RUBY_LIB_LOCATION
$: << LIB_LOCATION+'/ozones/models'
$: << LIB_LOCATION+'/ozones/lib'
$: << RUBY_LIB_LOCATION+"/cli"


##############################################################################
# Required libraries
##############################################################################
require 'rubygems'
require 'sinatra'

require 'yaml'
require 'rubygems'
require 'data_mapper'
require 'digest/sha1'
require 'OzonesServer'

##############################################################################
# Read configuration
##############################################################################
config_data=File.read(ETC_LOCATION+'/ozones-server.conf')
config=YAML::load(config_data)

db_type = config[:databasetype]

db_url = db_type + "://" + VAR_LOCATION + "/ozones.db"

##############################################################################
# DB bootstrapping
##############################################################################
if config[:dbdebug]
    DataMapper::Logger.new($stdout, :debug)
end

DataMapper.setup(:default, db_url)

require 'OZones'
require 'Auth'

DataMapper.finalize
DataMapper.auto_upgrade!

if Auth.all.size == 0
    if ENV['OZONES_AUTH'] && File.exist?(ENV['OZONES_AUTH'])
        credentials = IO.read(ENV['OZONES_AUTH']).strip.split(':')

        if credentials.length < 2
            warn "Authorization data malformed"
            exit -1
        end
        credentials[1] = Digest::SHA1.hexdigest(credentials[1])
        @auth=Auth.create({:name => credentials[0],
                           :password => credentials[1]})
        @auth.save
    else
        warn "oZones admin credentials not set, missing OZONES_AUTH file."
        exit -1
    end
else
    @auth=Auth.all.first
end

ADMIN_NAME = @auth.name
ADMIN_PASS = @auth.password

begin
    OZones::ProxyRules.new("apache",config[:htaccess])
rescue Exception => e
    warn e.message
    exit -1
end


##############################################################################
# Sinatra Configuration
##############################################################################
use Rack::Session::Pool, :key => 'ozones'
set :bind, config[:host]
set :port, config[:port]
set :show_exceptions, false

##############################################################################
# Helpers
##############################################################################
helpers do

    def authorized?
        session[:ip] && session[:ip]==request.ip ? true : false
    end

    def build_session
        auth = Rack::Auth::Basic::Request.new(request.env)

        if auth.provided? && auth.basic? && auth.credentials
            
            user      = auth.credentials[0]
            pass      = auth.credentials[1]
            sha1_pass = Digest::SHA1.hexdigest(pass)

            if user == ADMIN_NAME && sha1_pass == ADMIN_PASS
                session[:user] = user
                session[:ip]   = request.ip
                session[:key]  = Digest::SHA1.hexdigest("#{user}:#{pass}")

                session[:remember] = params[:remember]

                if params[:remember]
                    env['rack.session.options'][:expire_after] = 30*60*60*24
                end

                return [204, ""]
            else
                return [401, ""]
            end
        end

        return [401, ""]
    end

    def destroy_session
        session.clear
        return [204, ""]
    end
end

before do
    unless request.path=='/login' || request.path=='/'

        unless authorized?
            rc , msg = build_session

            if rc == 401
               halt 401 
            end
        end

        @OzonesServer = OzonesServer.new(session[:key])
        @pr           = OZones::ProxyRules.new("apache",config[:htaccess])
    end
end

after do
    unless request.path=='/login' || request.path=='/'
        unless session[:remember]
            if params[:timeout] == true
                env['rack.session.options'][:defer] = true
            else
                env['rack.session.options'][:expire_after] = 60*10
            end
        end
    end
end

##############################################################################
# HTML Requests
##############################################################################
get '/' do
    return  File.read(File.dirname(__FILE__)+
                      '/templates/login.html') unless authorized?

    time = Time.now + 60
    response.set_cookie("ozones-user",
                        :value=>"#{session[:user]}",
                        :expires=>time)

    File.read(File.dirname(__FILE__)+'/templates/index.html')
end

get '/login' do
    File.read(File.dirname(__FILE__)+'/templates/login.html')
end

##############################################################################
# Login
##############################################################################
post '/login' do
    build_session
end

post '/logout' do
    destroy_session
end

##############################################################################
# Config and Logs
##############################################################################
get '/config' do
    config
end

##############################################################################
# GET information
##############################################################################
get '/vdc' do
    @OzonesServer.get_vdcs
end

get '/vdc/:id' do
    @OzonesServer.get_vdc(params[:id])
end

get '/zone' do
    @OzonesServer.get_zones
end

get %r{/zone/([\d]+)/([\w]+)} do
    @OzonesServer.get_zone_pool(params[:captures][0], params[:captures][1])
end

get %r{/zone/([\d]+)} do
    @OzonesServer.get_zone(params[:captures].first)
end

get %r{/zone/([\w]+)} do
    @OzonesServer.get_zones_pool(params[:captures].first)
end

##############################################################################
# POSTs information
##############################################################################

post '/vdc' do
    @OzonesServer.create_vdc(request.body.read, @pr)
end

post '/zone' do
    @OzonesServer.create_zone(request.body.read, @pr)
end

##############################################################################
# Update Resource
##############################################################################
put '/vdc/:id' do
    @OzonesServer.update_vdc(params[:id], request.body.read)
end

##############################################################################
# Delete Resource
##############################################################################
delete '/vdc/:id' do
    @OzonesServer.delete_vdc(params[:id], @pr)
end

delete '/zone/:id' do
    @OzonesServer.delete_zone(params[:id], @pr)
end
