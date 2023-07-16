#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import json
import os
import shutil
import sys
from string import Template
#from datetime import date
import time
import re
import subprocess



if len(sys.argv) != 2:
    print("Usage: " + sys.argv[0] + " project_dir")
    exit(1)

project_dir = sys.argv[1]
print("Project: " + project_dir)

DATA_DIR = project_dir + "/data"
STATIC_DIR = project_dir + "/static"
MENU_CONFIG_FILE_NAME = "/menu.json"

OUTPUT_DIR = "generated__" + project_dir
if os.path.isdir(OUTPUT_DIR):
    shutil.rmtree(OUTPUT_DIR)
#os.makedirs(OUTPUT_DIR)
shutil.copytree(STATIC_DIR, OUTPUT_DIR)

menu_config_file = open(DATA_DIR + MENU_CONFIG_FILE_NAME, "r")
menu_config = json.load(menu_config_file)
menu_config_file.close()

localfile_re = re.compile("\$\{localfile:([^}]+)\}")



def get_file_contents(file_name):
    f = open(file_name, "r")
    s = f.read()
    f.close()
    return s

def create_menu(item):
    html = '\n<nav>\n<ul class="horizontal-menu">\n'
    current_file = item["file"]

    for menu_item in menu_config:
        if isinstance(menu_item, dict):
            # Page.
            if menu_item["file"] != current_file:
                html += '\t<li><a class="menu-item" href="' + menu_item["file"] + '.html">' + menu_item["label"] + '</a></li>\n'
        else:
            # Submenu.
            if menu_item[0]["file"] != current_file:
                html += '\t<li><a class="menu-item" href="' + menu_item[0]["file"] + '.html">' + menu_item[0]["label"] + '</a></li>\n'

    html += '</ul>\n</nav>\n\n'
    return html

def create_submenu(parent):
    html = '\n<nav>\n<ul class="horizontal-menu">\n'

    menu_len = len(menu_config)
    if menu_len < 3:
        raise Exception("Menu with not enough items.")

    html += '\t<li><a class="menu-item" href="' +            menu_config[0]["file"] + '.html">' +            menu_config[0]["label"] + '</a></li>\n'
    html += '\t<li><a class="menu-item" href="' +                    parent["file"] + '.html">' +                    parent["label"] + '</a></li>\n'
    html += '\t<li><a class="menu-item" href="' + menu_config[menu_len - 1]["file"] + '.html">' + menu_config[menu_len - 1]["label"] + '</a></li>\n'

    html += '</ul>\n</nav>\n\n'
    return html

def create_site_map():
    html = '\n<ul class="site-map">\n'

    for n, menu_item in enumerate(menu_config):
        if n > 0:
            if isinstance(menu_item, dict):
                # Page.
                html += '\t<li><a href="' + menu_item["file"] + '.html">' + menu_item["label"] + '</a></li>\n'
            else:
                # Submenu.
                html += ('\t<li>' + menu_item[0]["label"] + '\n\t\t<ul>\n')
                for submenu_item in menu_item[1:]:
                    if isinstance(submenu_item, dict):
                        html += '\t\t\t<li><a href="' + submenu_item["file"] + '.html">' + submenu_item["label"] + '</a>'
                        if len(submenu_item["description"]) > 0:
                            html += '<br>' + submenu_item["description"]
                    else:
                        html += '\t\t\t<li><a href="' + submenu_item[0]["file"] + '.html">' + submenu_item[0]["label"] + '</a>'
                        if len(submenu_item[0]["description"]) > 0:
                            html += '<br>' + submenu_item[0]["description"]
                    html += '</li>\n'
                html += '\t\t</ul>\n\t</li>\n'

    html += '</ul>\n\n'
    return html

def get_date_string(mtime):
    return time.strftime('%Y-%m-%d', time.localtime(mtime))

def create_page(item, level, number, parent):
    print("PAGE " + item["file"] + "|" + item["label"] + "|" + item["title"])

    if level > 0:
        menu = create_submenu(parent)
    else:
        menu = create_menu(item)

    content_file_name = DATA_DIR + "/content_" + item["file"] + ".txt"
    content = get_file_contents(content_file_name)

    # Check links to local files.
    localfile_iter = localfile_re.finditer(content)
    for match in localfile_iter:
        localfile_path = OUTPUT_DIR + "/" + match.group(1)
        if not os.path.isfile(localfile_path):
            raise Exception("The file {} does not exist.".format(localfile_path))
        else:
            content = re.sub("\$\{localfile:" + match.group(1) + "\}", match.group(1), content)

    if level == 0 and number == 0:
        content += create_site_map()

    date_string = get_date_string(os.path.getmtime(content_file_name))

    html_path = OUTPUT_DIR + "/" + item["file"] + ".html"
    page_file = open(html_path, "w")
    page_file.write(page_template.safe_substitute(title=item["title"], menu=menu, content=content, date=date_string))
    page_file.close()
    subprocess.call(["tidy", "-q", html_path])

def create_submenu_page(item, level, number, parent):
    print("PAGE (SUBMENU) " + item[0]["file"] + "|" + item[0]["label"] + "|" + item[0]["title"])

    if level > 0:
        menu = create_submenu(parent)
    else:
        menu = create_menu(item[0])

    content = ('<h1>' + item[0]["label"] + '</h1>\n\n'
               '<ul>\n')
    for submenu_item in item[1:]:
        if isinstance(submenu_item, dict):
            content += '<li><a href="' + submenu_item["file"] + '.html">' + submenu_item["label"] + '</a>'
            if len(submenu_item["description"]) > 0:
                content += '<br>' + submenu_item["description"]
        else:
            content += '<li><a href="' + submenu_item[0]["file"] + '.html">' + submenu_item[0]["label"] + '</a>'
            if len(submenu_item[0]["description"]) > 0:
                content += '<br>' + submenu_item[0]["description"]
        content += '</li>\n'
    content += '</ul>\n\n'

    #date_string = date.today().isoformat()
    date_string = get_date_string(os.path.getmtime(DATA_DIR + MENU_CONFIG_FILE_NAME))

    html_path = OUTPUT_DIR + "/" + item[0]["file"] + ".html"
    page_file = open(html_path, "w")
    page_file.write(page_template.safe_substitute(title=item[0]["title"], menu=menu, content=content, date=date_string))
    page_file.close()
    subprocess.call(["tidy", "-q", html_path])

def process_menu_item(item, level, number, parent):
    if isinstance(item, dict):
        # Page.
        create_page(item, level, number, parent)
    else:
        # Submenu.
        create_submenu_page(item, level, number, parent)
        for n, submenu_item in enumerate(item[1:]):
            process_menu_item(submenu_item, level + 1, n, item[0])



page_template = Template(get_file_contents(DATA_DIR + "/page_template.txt"))

for n, menu_item in enumerate(menu_config):
    process_menu_item(menu_item, 0, n, None)
