import sphinx_bootstrap_theme
import os
import sys

sys.path.append(os.path.abspath("./extensions"))

project = 'boost-wintls'
copyright = '2021, Kasper Laudrup'
author = 'Kasper Laudrup'

master_doc = 'index'

extensions = ['sphinx.ext.autosectionlabel',
              'breathe',
              'toctree_elements',
              'remove_inline_specifier',
              ]

highlight_language = 'c++'

primary_domain = 'cpp'

templates_path = ['templates']

html_static_path = ['static']

html_title = 'boost.wintls'

html_css_files = [
  project + '.css',
]

html_theme = 'bootstrap'
html_theme_path = sphinx_bootstrap_theme.get_html_theme_path()

html_theme_options = {
    'bootswatch_theme': 'flatly',
    'navbar_title': html_title
}

html_last_updated_fmt = ''

breathe_default_project = 'boost-wintls'
