local gltf = {}

gltf.init = function(self)
	-- Add initialization code here
	-- Learn more: https://defold.com/manuals/script/
	-- Remove this function if not needed
	self.gltf_pred = render.predicate({"gltf"})
end

gltf.final = function(self)
	-- Add finalization code here
	-- Learn more: https://defold.com/manuals/script/
	-- Remove this function if not needed
end

gltf.update = function(self, dt)
	-- Add update code here
	-- Learn more: https://defold.com/manuals/script/
	-- Remove this function if not needed

	render.draw(self.gltf_pred)
end

gltf.on_message = function(self, message_id, message, sender)
	-- Add message-handling code here
	-- Learn more: https://defold.com/manuals/message-passing/
	-- Remove this function if not needed
end

gltf.on_reload = function(self)
	-- Add reload-handling code here
	-- Learn more: https://defold.com/manuals/hot-reload/
	-- Remove this function if not needed
end

return gltf