set_project("gf2-tileset-fixer")
set_version("1.0.0")

add_repositories("gf-repo https://github.com/GamedevFramework/xmake-repo")

add_requires("gamedevframework2")
add_requires("nlohmann_json")

if is_kind("static") then
    add_requireconfs("gamedevframework2", {system = false, configs = {shared = false}})
    add_requireconfs("gamedevframework2.**", {system = false, configs = {shared = false}})
end

add_rules("mode.debug", "mode.releasedbg", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "$(builddir)"})

if is_mode("sanitizers") then
    set_symbols("debug")
    set_optimize("none")
    set_policy("build.sanitizer.address", true)
    set_policy("build.sanitizer.undefined", true)
end

set_policy("build.warning", true)
set_warnings("allextra")
set_languages("cxx20")
set_encodings("utf-8")

if is_plat("windows") then
  add_cxflags("/wd4251") -- Disable warning: class needs to have dll-interface to be used by clients of class blah blah blah
  add_defines("NOMINMAX", "_CRT_SECURE_NO_WARNINGS")
end

target("gf2-tileset-fixer")
    set_kind("binary")
    add_files("code/main.cc")
    add_packages("gamedevframework2")
    add_packages("nlohmann_json")
    set_rundir("$(projectdir)")
    if is_plat("windows") then
        add_files("app.manifest")
        add_ldflags("/subsystem:console")
    end
