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
    'description': 'Python bindings for the libvirt API',
    'github_button': False,
}


def _needs_literal_docstring(lines):
    """Here comes a little bit of magic.
    This function returns True if docstring content is unsafe as reStructuredText,
    it will tell Sphinx to use the docstring as is, without any formatting.

    Docs from C API descriptions are not always valid reStructuredText,
    these are valid for Python help() but are not docutils.
    """
    in_field = False
    for line in lines:
        if line.startswith(':'):
            in_field = True
            continue
        if in_field:
            stripped = line.lstrip()
            if stripped.startswith(('-', '*')) and line[:1].isspace():
                return True
            if line and not line[0].isspace() and not line.startswith(':'):
                in_field = False
        if line.startswith((' ', '\t')):
            return True
    return False


def process_docstring(app, what, name, obj, options, lines):
    if not lines or not _needs_literal_docstring(lines):
        return
    body = list(lines)
    lines[:] = ['::', ''] + [('    ' + line if line else '') for line in body]


def setup(app):
    app.connect('autodoc-process-docstring', process_docstring)
