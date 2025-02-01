#!/usr/bin/env python3

# This script generates the compile.md manual page from the GitHub build.yml file.
#
# (c) 2024-2025 Daniel Markstedt <daniel@mindani.net>
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

import re
import yaml

linebreak_escape_pattern = r'\\\n\s+'

output_file = "Compilation.md"

with open('../../.github/workflows/build.yml', 'r') as file:
  workflow = yaml.safe_load(file)

markdown = [
  "# Compile Netatalk from Source",
  "",
  "This appendix describes how to compile Netatalk from source for specific operating systems.",
  "Before starting, please read through the Installation chapter first.",
  "You need to have a copy of Netatalk's source code before proceeding.",
  "",
  "Please note that these guides are automatically generated, and may not be optimized for your system.",
  "Also, the steps for launching Netatalk are incomplete for some OSes, because of technical constraints.",
  "",
  "# Operating Systems",
  "",
]

for key, value in workflow["jobs"].items():
  # Skip the SonarCloud job
  if value["name"] != "Static Analysis":
    markdown += [
      "## " + value["name"],
      "",
    ]
    for step in value["steps"]:
      # Skip unnamed steps
      if "name" not in step:
        continue
      # Skip GitHub actions steps irrelevant to documentation
      if "uses" in step and step["uses"].startswith("actions/"):
        continue
      # Strip out line break escape sequences
      if "run" in step:
        step["run"] = re.sub(linebreak_escape_pattern, "", step["run"])
      if "with" in step:
        step["with"]["prepare"] = re.sub(linebreak_escape_pattern, "", step["with"]["prepare"])
        step["with"]["run"] = re.sub(linebreak_escape_pattern, "", step["with"]["run"])

      # The vmactions jobs have a different structure
      if "uses" in step and step["uses"].startswith("vmactions/"):
        markdown += [
            "Install required packages",
            "",
            "```",
            step["with"]["prepare"].rstrip("\n"),
            "```",
            "",
            "Configure and build",
            "",
            "```",
            step["with"]["run"].rstrip("\n"),
            "```",
            "",
        ]
      else:
        markdown += [
            step["name"],
            "",
            "```",
            step["run"].rstrip("\n"),
            "```",
            "",
        ]

with open(output_file, 'w') as file:
  file.write("\n".join(markdown))

print("Wrote to " + output_file)
