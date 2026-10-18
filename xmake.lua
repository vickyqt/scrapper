---@diagnostic disable: undefined-global

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".", lsp = "clangd"})
set_defaultmode("debug")              -- debug unless you choose otherwise

set_languages("c++23")

add_requires("vcpkg::curl")
add_requires("vcpkg::libxml2")

target("scrapper")
set_kind("binary")
add_files("src/*.cpp")
add_packages("vcpkg::curl", "vcpkg::libxml2")
