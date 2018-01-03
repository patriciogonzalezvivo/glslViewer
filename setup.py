from setuptools import setup

setup(
    name = 'glslviewer',
    url = 'https://github.com/patriciogonzalezvivo/glslViewer',
    version = 1.6,
    author='Patricio Gonzalez Vivo',
    author_email='patriciogonzalezvivo@gmail.com',
    description = 'GlslViewer wrapper for Python 2 and 3',
    long_description = '''
This is a simple wrapper for GlslViewer. It communicate with the sandbox throught CIN and COUT of GlslViewer instances
''',
    license='BSD',
    package_dir = {'': 'python'},
    packages = ['glslviewer']
)