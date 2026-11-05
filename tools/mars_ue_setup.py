"""Create MarsMap's Unreal assets: import the baked textures and build the globe material.

Run after tools/mars_prep.py (UE is the engine folder):
    $UE/Engine/Binaries/Mac/UnrealEditor-Cmd MarsMap/MarsMap.uproject -run=pythonscript -script="$PWD/tools/mars_ue_setup.py"
"""
import os

import unreal

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary

tasks = []
for name in ("T_MarsMola", "T_MarsNatural"):
    t = unreal.AssetImportTask()
    t.filename = os.path.join(ROOT, "dem", "mars", name + ".png")
    t.destination_path = "/Game/Mars"
    t.automated = True
    t.replace_existing = True
    t.save = True
    tasks.append(t)
tools.import_asset_tasks(tasks)
for t in tasks:
    assert t.imported_object_paths, "import failed: " + t.filename

if eal.does_asset_exist("/Game/Mars/M_Globe"):
    eal.delete_asset("/Game/Mars/M_Globe")
m = tools.create_asset("M_Globe", "/Game/Mars", unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("two_sided", True)

tex = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -500, 0)
tex.set_editor_property("parameter_name", "Color")
tex.set_editor_property("texture", unreal.load_asset("/Game/Mars/T_MarsMola"))
mel.connect_material_property(tex, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)

# A little self-glow so the night side is dim instead of black
amb = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -500, 300)
amb.set_editor_property("parameter_name", "Ambient")
amb.set_editor_property("default_value", 0.04)
mul = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -200, 300)
mel.connect_material_expressions(tex, "RGB", mul, "A")
mel.connect_material_expressions(amb, "", mul, "B")
mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

for value, prop, y in ((1.0, unreal.MaterialProperty.MP_ROUGHNESS, 500), (0.1, unreal.MaterialProperty.MP_SPECULAR, 600)):
    c = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -200, y)
    c.set_editor_property("r", value)
    mel.connect_material_property(c, "", prop)

mel.recompile_material(m)
eal.save_asset("/Game/Mars/M_Globe")
unreal.log("MarsMap assets ready")
