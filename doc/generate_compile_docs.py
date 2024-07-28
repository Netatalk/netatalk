#!/usr/bin/env python3

# This script generates the compile.xml manual page from the GitHub build.yml file.
#
# (c) 2024 Daniel Markstedt <daniel@mindani.net>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#

import datetime
import re
import xmltodict
import yaml

now = datetime.datetime.now()
date_time = now.strftime("%Y-%m-%d")

lang_en = {
  "title_1": "Compile Netatalk from Source",
  "title_2": "Overview",
  "title_3": "Operating Systems",
  "heading_1": "Install required packages",
  "heading_2": "Configure and build",
  "para_1": "This appendix describes how to compile Netatalk from source for specific operating systems.",
  "para_2": "Please note that the steps below are automatically generated, and may not be optimized for your system.",
  "para_3": "Choose one of the build systems: Autotools or Meson. Test steps are optional.",
}

output_en = "./manual/compile.xml"

with open('../.github/workflows/build.yml', 'r') as file:
  workflow = yaml.safe_load(file)

apt_packages_pattern = r'\$\{\{\senv\.APT_PACKAGES\s\}\}'
apt_packages = workflow["env"]["APT_PACKAGES"]

def generate_docbook(strings, output_file):
  docbook = {
    "appendix": {
      "@id": "compile",
      "appendixinfo": [
        {
          "pubdate": date_time
        },
      ],
      "title": strings["title_1"],
      "sect1": [
        {
          "@id": "compile-overview",
          "title": strings["title_2"],
        },
        {
          "para": [
            strings["para_1"],
            strings["para_2"],
            strings["para_3"],
          ],
        },
        {
          "@id": "compile-os",
          "title": strings["title_3"],
        },
        {
          "sect2": [],
        }
      ],
    },
  }

  for key, value in workflow["jobs"].items():
    sections = {}
    # Skip the SonarCloud job
    if value["name"] != "Static Analysis":
      sections["@id"] = key
      sections["title"] = value["name"]
      para = []
      for step in value["steps"]:
        # Skip unnamed steps
        if "name" not in step:
          continue
        # Skip GitHub actions steps irrelevant to documentation
        if "uses" in step and step["uses"].startswith("actions/"):
          continue
        # Substitute the APT_PACKAGES variable with the actual list of packages
        if "run" in step and re.search(apt_packages_pattern, step["run"]):
          step["run"] = re.sub(apt_packages_pattern, apt_packages, step["run"])

        # The vmactions jobs have a different structure
        if "uses" in step and step["uses"].startswith("vmactions/"):
          para.append(strings["heading_1"])
          para.append({"screen": step["with"]["prepare"]})
          para.append(strings["heading_2"])
          para.append({"screen": step["with"]["run"]})
        else:
          para.append(step["name"])
          para.append({"screen": step["run"]})

      sections["para"] = para

    docbook["appendix"]["sect1"][3]["sect2"].append(sections)

  xml = xmltodict.unparse(docbook, pretty=True)

  with open(output_file, 'w') as file:
    file.write(xml)

  print("Wrote to " + output_file)

generate_docbook(lang_en, output_en)
