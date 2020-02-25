timer_generator = require "timer_generator"

config_manager = require "trans_cli_config_manager"
io_manager = require "trans_cli_io_manager"

file_reader_manager = require "file_reader_manager"
file_writer_manager = require "file_writer_manager"

trans_cli_upload_manager = require "trans_cli_upload_manager"
trans_cli_manager = require "trans_cli_download_manager"
trans_cli_manager = require "trans_cli_sync_manager"
	
function init_server(vars)
	
	init_ret = 0
	
	--print("begin to init modules with lua")
	
	timer_generator.init(vars)
	if init_ret ~= 0 then
		return init_ret
	end
	
	config_manager.init(vars)
	if init_ret ~= 0 then
		return init_ret
	end
	
	io_manager.init(vars)
	if init_ret ~= 0 then
		return init_ret
	end
	
	file_reader_manager.init(vars)
	if init_ret ~= 0 then
		return init_ret
	end
	
	file_writer_manager.init(vars)
	if init_ret ~= 0 then
		return init_ret
	end
	
	trans_cli_upload_manager.init(vars)
	if init_ret ~= 0 then
		return init_ret
	end	
	
	trans_cli_download_manager.init(vars)
	if init_ret ~= 0 then
		return init_ret
	end	

	trans_cli_sync_manager.init(vars)
	if init_ret ~= 0 then
		return init_ret
	end	
	
	--print("end to init modules with lua")
	
	return init_ret
end


function exit_server(vars)

	exit_ret = 0
	
	timer_generator.exit(vars)
	
	config_manager.exit(vars)
	io_manager.exit(vars)
	
	file_reader_manager.exit(vars)
	file_writer_manager.exit(vars)
	
	trans_cli_upload_manager.exit(vars)
	trans_cli_download_manager.exit(vars)
	trans_cli_sync_manager.exit(vars)
	
	return exit_ret
end