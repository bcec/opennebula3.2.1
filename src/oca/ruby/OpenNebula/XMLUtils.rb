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


module OpenNebula

    begin
        require 'nokogiri'
        NOKOGIRI=true
    rescue LoadError
        NOKOGIRI=false
    end

    begin
        require 'rexml/formatters/pretty'
        REXML_FORMATTERS=true
    rescue LoadError
        REXML_FORMATTERS=false
    end

    # The XMLElement class provides an abstraction of the underlying
    # XML parser engine. It provides XML-related methods for the Pool and
    # PoolElement classes
    class XMLElement

        # xml:: _opaque xml object_ an xml object as returned by build_xml
        def initialize(xml=nil)
            @xml = xml
        end

        # Initialize a XML document for the element
        # xml:: _String_ the XML document of the object
        # root_element:: _String_ Base xml element
        def initialize_xml(xml, root_element)
            @xml = XMLElement.build_xml(xml, root_element)

            if OpenNebula.is_error?(@xml)
                @xml = nil
            else
                if NOKOGIRI
                    if @xml.size == 0
                        @xml = nil
                    end
                else
                    if @xml.name != root_element
                        @xml = nil
                    end
                end
            end
        end

        # Builds a XML document
        # xml:: _String_ the XML document of the object
        # root_element:: _String_ Base xml element
        # [return] _XML_ object for the underlying XML engine
        def self.build_xml(xml, root_element)
            begin
                if NOKOGIRI
                    doc = Nokogiri::XML(xml).xpath("/#{root_element}")
                else
                    doc = REXML::Document.new(xml).root
                end
            rescue Exception => e
                return OpenNebula::Error.new(e.message)
            end

            return doc
        end
        # Extract an element from the XML description of the PoolElement.
        # key::_String_ The name of the element
        # [return] _String_ the value of the element
        # Examples:
        #   ['VID'] # gets VM id
        #   ['HISTORY/HOSTNAME'] # get the hostname from the history
        def [](key)
            if NOKOGIRI
                element=@xml.xpath(key.to_s)

                if element.size == 0
                    return nil
                end
            else
                element=@xml.elements[key.to_s]
            end

            if element
                element.text
            end
        end

        # Gets an array of text from elemenets extracted
        # using  the XPATH  expression passed as filter
        def retrieve_elements(filter)
            elements_array = Array.new

            if NOKOGIRI
                @xml.xpath(filter.to_s).each { |pelem|
                    elements_array << pelem.text if pelem.text
                 }
            else
                @xml.elements.each(filter.to_s) { |pelem|
                    elements_array << pelem.text if pelem.text
                }
            end

            if elements_array.size == 0
                return nil
            else
                return elements_array
            end

        end

        # Gets an attribute from an elemenT
        # key:: _String_ xpath for the element
        # name:: _String_ name of the attribute
        def attr(key,name)
            value = nil

            if NOKOGIRI
                element=@xml.xpath(key.to_s.upcase)
                if element.size == 0
                    return nil
                end

                attribute = element.attr(name)

                value = attribute.text if attribute != nil
            else
                element=@xml.elements[key.to_s.upcase]

                value = element.attributes[name] if element != nil
            end

            return value
        end

        # Iterates over every Element in the XPath and calls the block with a
        # a XMLElement
        # block:: _Block_
        def each(xpath_str,&block)
            if NOKOGIRI
                @xml.xpath(xpath_str).each { |pelem|
                    block.call XMLElement.new(pelem)
                }
            else
                @xml.elements.each(xpath_str) { |pelem|
                    block.call XMLElement.new(pelem)
                }
            end
        end

        def each_xpath(xpath_str,&block)
            if NOKOGIRI
                @xml.xpath(xpath_str).each { |pelem|
                    block.call pelem.text
                }
            else
                @xml.elements.each(xpath_str) { |pelem|
                    block.call pelem.text
                }
            end
        end

        def name
            @xml.name
        end

        def text
            if NOKOGIRI
                @xml.content
            else
                @xml.text
            end
        end

        def has_elements?(xpath_str)
            if NOKOGIRI
                element = @xml.xpath(xpath_str.to_s.upcase)
                return element != nil && element.children.size > 0
            else
                element = @xml.elements[xpath_str.to_s]
                return element != nil && element.has_elements?
            end
        end

        def template_str(indent=true)
            template_like_str('TEMPLATE', indent)
        end

        def template_like_str(root_element, indent=true, xpath_exp=nil)
            if NOKOGIRI
                xml_template=@xml.xpath(root_element).to_s
                rexml=REXML::Document.new(xml_template).root
            else
                rexml=@xml.elements[root_element]
            end

            if indent
                ind_enter="\n"
                ind_tab='  '
            else
                ind_enter=''
                ind_tab=' '
            end

            str=rexml.elements.collect(xpath_exp) {|n|
                if n.class==REXML::Element
                    str_line=""
                    if n.has_elements?
                        str_line << n.name << "=[" << ind_enter

                        str_line << n.collect {|n2|
                            if n2 && n2.class==REXML::Element
                                str = ""
                                str << ind_tab << n2.name << '='
                                str << attr_to_str(n2.text) if n2.text
                                str
                            end
                        }.compact.join(','+ind_enter)
                        str_line<<" ]"
                    else
                        str_line << n.name << '=' << attr_to_str(n.text.to_s)
                    end
                    str_line
                end
            }.compact.join("\n")

            str
        end

        def to_xml(pretty=false)
            if NOKOGIRI && pretty
                str = @xml.to_xml
            elsif REXML_FORMATTERS && pretty
                str = String.new

                formatter = REXML::Formatters::Pretty.new
                formatter.compact = true

                formatter.write(@xml,str)
            else
                str = @xml.to_s
            end

            return str
        end

        def to_hash(hash={}, element=nil)
            element ||= @xml.document.root

            if NOKOGIRI
                array = element.children
                if array.length==1 and (array.first.text? or array.first.cdata?)
                    r = array.first.text
                else
                    r = {}
                    array.each { |c|
                        if c.element?
                            to_hash(r, c)
                        end
                    }
                end
            else
                r = {}
                if element.has_elements?
                    element.each_element { |c| to_hash(r, c) }
                elsif element.has_text?
                    r = element.text
                end
            end

            key = element.name
            if hash.has_key?(key)
                if hash[key].instance_of?(Array)
                    hash[key] << r
                else
                    hash[key] = [hash[key], r]
                end
            else
                hash[key] = r
            end

            hash
        end

    private
        def attr_to_str(attr)
            attr.gsub!('"',"\\\"")

            if attr.match(/[=,' ']/)
                return '"' + attr + '"'
            end

            return attr
        end
    end

    # The XMLUtilsPool module provides an abstraction of the underlying
    # XML parser engine. It provides XML-related methods for the Pools
    class XMLPool < XMLElement

        def initialize(xml=nil)
            super(xml)
        end

        #Executes the given block for each element of the Pool
        #block:: _Block_
        def each_element(block)
            if NOKOGIRI
                @xml.xpath(
                    "#{@element_name}").each {|pelem|
                    block.call self.factory(pelem)
                }
            else
                @xml.elements.each(
                    "#{@element_name}") {|pelem|
                    block.call self.factory(pelem)
                }
            end
        end
    end

end
