import importlib
from pathlib import Path
import os
import sys

# this is just a glorified lazy loader
class GlslViewerMetaClass(type):
    module = None

    def __getattr__(cls, name):
        if cls.module is None:
            pylib_dir = os.path.abspath( os.path.join( Path(__file__).parent.resolve(), '../build/' ) )

            # sanity checks
            if pylib_dir is None:
                return
            
            if not Path(pylib_dir).exists():
                return
            
            if not Path(pylib_dir).is_dir():
                return

            # add to sys.path
            if pylib_dir not in sys.path:
                sys.path.append(pylib_dir)

        # import and return
        cls.module = importlib.import_module('PyGlslViewer')
        return getattr(cls.module, name)


class GlslViewer(metaclass=GlslViewerMetaClass):
    def __getattr__(self, name):
        return getattr(type(self), name)
