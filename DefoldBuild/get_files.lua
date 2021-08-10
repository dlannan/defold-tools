-- Get files from defold stable site -- needed for building
local json = require("json")

-- Remove previous downloads
os.execute("rm index.html*")
-- First get the index
os.execute("wget https://d.defold.com/stable/")

-- Open index and get all the downloads
local fh = io.open("index.html", "r")
local html = fh:read("*a")
fh:close()

-- Find the data in the html
local files = string.match(html, "var model = {(.-)};")
-- print(files)

local tbl = json.decode("{"..files.."}")

print("Fetching release: "..tbl.releases[1].tag)
print(tbl.releases)

for k,v in pairs(tbl.releases[1]) do print(k,v) end
local newdir = "release-"..tbl.releases[1].tag
os.execute("mkdir "..newdir.." -p")

for k,v in pairs(tbl.releases[1].files) do 
	local newpath = string.match(v.path, tbl.releases[1].sha1.."(.*)/")
	os.execute("wget https://d.defold.com"..v.path.." -P ./"..newdir..newpath)	
end

-- Add links to latest
os.execute("rm latest")
os.execute("ln -s "..newdir.." latest")

