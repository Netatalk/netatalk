#!/usr/bin/env python3

# This script generates the compile.xml manual page from the GitHub build.yml file.

import re
import yaml
import xmltodict

lang_en = {
  "title_1": "Compile Netatalk from Source",
  "title_2": "Overview",
  "title_3": "Operating Systems",
  "heading_1": "Install required packages",
  "heading_2": "Configure and build",
  "para_1": "This section describes how to compile Netatalk from source for specific operating systems.",
  "para_2": "Please note that this chapter is automatically generated and may not be optimized for your system.",
  "para_3": "Choose either Autotools or Meson as the build system. Test steps are optional.",
}
lang_jp = {
  "title_1": "Netatalk をソースコードからコンパイルする",
  "title_2": "概要",
  "title_3": "オペレーティング システム一覧",
  "heading_1": "必要なパッケージをインストールする",
  "heading_2": "コンフィグレーションとビルド",
  "para_1": "本付録では、特定のオペレーティング システムのソースから Netatalk をコンパイルする方法について説明する。",
  "para_2": "以下文章は自動的に生成されるため、お使いのシステムに最適化されていない可能性があることに注意してください。",
  "para_3": "ビルド システムとして Autotools と Meson から選択する。 テスト手順は任意である。",
}

output_en = "compile.xml"
output_jp = "../ja/manual/compile.xml"

with open('../../.github/workflows/build.yml', 'r') as file:
  workflow = yaml.safe_load(file)

apt_packages_pattern = r'\$\{\{\senv\.APT_PACKAGES\s\}\}'
apt_packages = workflow["env"]["APT_PACKAGES"]

def generate_docbook(strings, output_file):
  docbook = {
    "appendix": {
      "@id": "compile",
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
          ],
        },
        {
          "@id": "compile-os",
          "title": strings["title_3"],
        },
        {
          "para": [
            strings["para_3"],
          ],
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
generate_docbook(lang_jp, output_jp)
