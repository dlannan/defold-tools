# defold-tools
A suite of tools for use with Defold


## GLTF Loader
This loader uses TinyGLTF to load in models at runtime. 
HTML is probably not going to work, but others should be fine. 
Missing:
- No texture loading or shader loading yet. 
- No anim
- No PBR (will make some shaders in Defold for this)
- Simplistic single mesh support. Multi-mesh might be a little complicated.

General use:
1. Create a temporary mesh (see assets/gotemplate folder) with a buffer and game object.
2. Place the gameobject into the scene.
3. Call: ```gltf:load(fname, goname, "temp")```  where fname is the glTF filename (with path), goname is the gameobject created and "temp" is the mesh component name within the gameobject. 
4. The gameobject will have its vertices filled with the information from the gltf file.


