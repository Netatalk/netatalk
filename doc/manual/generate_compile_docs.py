#!/usr/bin/env python3

# This script generates the compile.xml manual page from the GitHub build.yml file.

import re
import yaml
import xmltodict

def main():
  with open('../../.github/workflows/build.yml', 'r') as file:
    workflow = yaml.safe_load(file)

  apt_packages_pattern = r'\$\{\{\senv\.APT_PACKAGES\s\}\}'
  apt_packages = workflow["env"]["APT_PACKAGES"]

  docbook = {
    "appendix": {
      "@id": "compile",
      "title": "Compile Netatalk from Source",
      "sect1": [
        {
          "@id": "compile-overview",
          "title": "Overview",
        },
        {
          "para": [
            """
            This section describes how to compile Netatalk from source for specific operating systems.
            """,
            """
            The Netatalk project is in the process of transitioning the build system
            from GNU Autotools to Meson.
            During the transitional period,
            the documentation will contain both Autotools and Meson build instructions.
            """,
            """
            Please note that this document is automatically generated from the GitHub workflow YAML file.
            This may or may not be the most optimal way to build Netatalk for your system.
            """,
          ],
        },
        {
          "@id": "compile-os",
          "title": "Operating Systems",
        },
        {
          "para": [
            """
            Note: You only have to use execute the steps for one of the build systems, not both.
            Additionally, the test steps are entirely optional for compiling from source.
            """,
          ],
          "sect2": [],
        }
      ],
    },
  }

  for key, value in workflow["jobs"].items():
    sections = {}
    # Skip the SonarQube job
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
          para.append("Install dependencies")
          para.append({"screen": step["with"]["prepare"]})
          para.append("Configure and build")
          para.append({"screen": step["with"]["run"]})
        else:
          para.append(step["name"])
          para.append({"screen": step["run"]})

      sections["para"] = para

    docbook["appendix"]["sect1"][3]["sect2"].append(sections)

  xml = xmltodict.unparse(docbook, pretty=True)

  with open('compile.xml', 'w') as file:
    file.write(xml)

  print("Wrote compile.xml")

if __name__ == '__main__':
    main()
