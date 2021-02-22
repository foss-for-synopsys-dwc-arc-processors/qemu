require 'rubygems'
#require 'graphviz'

project_root = File.dirname(File.absolute_path(__FILE__))
Dir.glob(project_root + "/parsers/*.rb").each do |file|
  require file
end

Dir.glob(project_root + "/modules/*.rb").each do |file|
  require file
end

Dir.glob(project_root + "/classes/*.rb").each do |file|
  require file
end
