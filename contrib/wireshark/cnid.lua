-- 
-- Netatalk DBD protocol 
-- wireshark -X lua_script:cnid.lua
-- don't forget to comment out the line disable_lua = true; do return end;
-- in /etc/wireshark/init.lua

-- global environment
local b = _G

-- declare our protocol
local dbd_proto = Proto("dbd","Netatalk Dbd Wire Protocol")

local cmd = ProtoField.uint32("dbd.cmd", "Request") -- , base.HEX
local len = ProtoField.uint32("dbd.name.len", "Name Length")
local filename = ProtoField.string("dbd.name", "Name")
local error = ProtoField.uint32("dbd.error", "Error code")
local cnid = ProtoField.uint32("dbd.cnid", "Cnid")
local did  = ProtoField.uint32("dbd.did", "Parent Directory Id")
local dev  = ProtoField.uint64("dbd.dev", "Device number")
local ino  = ProtoField.uint64("dbd.ino", "Inode number")
local type = ProtoField.uint32("dbd.type", "File type")

dbd_proto.fields = {cmd, error, cnid, did, dev, ino, type, filename, len}

--- Request list
local Cmd = { [3] = "add", 
	      [4] = "get", 
	      [5] = "resolve", 
	      [6] = "lookup", 
	      [7] = "update", 
	      [8] = "delete", 
	      [11] = "timestamp" 
	    }

--- display a filename 
local function fname(buffer, pinfo, tree, len, ofs)

    pinfo.cols.info:append(" Name=" .. buffer(ofs +4, len):string())

    local subtree = tree:add(buffer(ofs, len +4), buffer(ofs +4, len):string())
    subtree:add(filename, buffer(ofs +4, len))

    return subtree
end

-- create a function to dissect it
function dbd_proto.dissector(buffer, pinfo, tree)


    pinfo.cols.protocol = "DBD"

    local subtree = tree:add(dbd_proto,buffer(),"Netatalk DBD Wire Protocol")

    if pinfo.dst_port == 4700 then
    	    pinfo.cols.info = "Query"
	    local val = buffer(0,4):uint()
	    local item = subtree:add(cmd, buffer(0,4))
	    if Cmd[val] then
	    	item:append_text(" (" .. Cmd[val] .. ")")
	    	pinfo.cols.info = Cmd[val]

	    	local val = buffer(4,4):uint()
	    	if val ~= 0 then
	    		pinfo.cols.info:append(" Cnid=" .. val)
		end
	    	subtree:add(cnid, buffer(4, 4))
	    	subtree:add(dev, buffer(8, 8))
	    	subtree:add(ino, buffer(16, 8))
	    	subtree:add(type, buffer(24, 4))

	    	local val = buffer(28,4):uint()
	    	if val ~= 0 then
	    	   pinfo.cols.info:append(" Did=" .. val)
		end
	    	subtree:add(did, buffer(28, 4))

	    	local val = buffer(36,4):uint()
	    	if val ~= 0 then
	    	   item = fname(buffer, pinfo, subtree, val, 36)
	    	   item:add(len, buffer(36, 4))
	    		
	    	end
	    end
    else
    	    pinfo.cols.info = "Reply"

    	    local rply = {}
    	    
	    local val = buffer(0,4):uint()
    	    rply.error = val
	    subtree:add(error, buffer(0,4))
	    if val ~= 0 then
	    	pinfo.cols.info:append(" Error=" .. val)
	    end

	    val = buffer(4,4):uint()
	    rply.cnid = val
	    subtree:add(cnid, buffer(4,4))
	    if val ~= 0 then
	    	pinfo.cols.info:append(" Cnid=" .. val)
	    end

	    val = buffer(8,4):uint()
	    rply.did = val
	    subtree:add(did, buffer(8,4))
	    if val ~= 0 then
	    	pinfo.cols.info:append(" Did=" .. val)
	    end

	    val = buffer(16,4):uint()
    	    rply.len = val
	    
	    if rply.error == 0 and rply.did ~= 0 then
	       subtree = fname(buffer, pinfo, subtree, val, 16)
	       subtree:add(len, buffer(16,4))
	    end
    end
end

-- load the tcp.port table
local tcp_table = DissectorTable.get("tcp.port")
-- register our protocol 
tcp_table:add(4700, dbd_proto)
