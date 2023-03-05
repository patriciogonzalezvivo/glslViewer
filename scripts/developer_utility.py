import os
import sys
import pkgutil
import importlib

def setup_addon_modules(path, package_name, reload):
    """Imports and reloads all modules in this addon.
    Individual modules can define a :code:`__reload_order_index__` property
    which will be used to reload the modules in a specific order. The default
    is 0.
    """

    def get_submodule_names(path=path[0], root=""):
        module_names = []
        for importer, module_name, is_package in pkgutil.iter_modules([path]):
            if is_package:
                sub_path = os.path.join(path, module_name)
                sub_root = root + module_name + "."
                module_names.extend(get_submodule_names(sub_path, sub_root))
            else:
                module_names.append(root + module_name)
        return module_names

    def import_submodules(names):
        modules = []
        for name in names:
            modules.append(importlib.import_module("." + name, package_name))
        return modules

    def reload_modules(modules):
        modules.sort(
            key=lambda module: getattr(module, "__reload_order_index__", 0)
        )
        for module in modules:
            importlib.reload(module)

    names = get_submodule_names()
    modules = import_submodules(names)
    
    if reload:
        reload_modules(modules)
    return modules