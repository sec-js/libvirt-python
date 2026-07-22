import os
import sys

sys.path.insert(0, os.path.abspath('../build'))
sys.path.insert(0, os.path.abspath('..'))

project = 'libvirt-python'
author = 'Libvirt Maintainers'

with open(os.path.abspath('../VERSION')) as f:
    version = f.read().strip()
release = version

# Libvirt major version tracks the year as major + 2014 (e.g. 12 → 2026)
copyright = '%d, Libvirt Maintainers' % (int(version.split('.')[0]) + 2014)

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.viewcode',
]

autodoc_member_order = 'groupwise'
autodoc_default_options = {
    'members': True,
    'undoc-members': True,
    'show-inheritance': True,
}

html_theme = 'alabaster'
html_theme_options = {
    'description': 'Python bindings for the libvirt virtualization API',
    'github_button': False,
}
