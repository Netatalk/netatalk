---
type: Reading Guide
title: "OKF Knowledge Graph"
description: "Where the graph is, how to navigate it, the rules for reading it (trust an edge that exists, never the absence of one) and recipes over graph.json."
tags: ["guide"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T02:20:26+11:00 }
---

# OKF Knowledge Graph

The knowledge graph is a map of the Netatalk sources drawn by Doxygen and
published as an [Open Knowledge Format](https://github.com/GoogleCloudPlatform/open-knowledge-format/blob/main/SPEC.md)
v0.2 bundle: one markdown concept per source file and per subsystem, with
include, call, table, dispatch, variable-use and documented-reference edges
as markdown links and in a `graph.json`. A CI job rebuilds it on every merge
to `main` that touches the scanned sources and publishes it under `okf/` on
the `gh-pages` branch. Every concept is machine-generated and carries
`generated` without `verified`, so the OKF trust tier is unverified
throughout. This page is shipped inside the bundle as `reading.md`; its
source is `doc/developer/okf-knowledge-graph.md`.

## Where the graph is

- Browse: `https://github.com/Netatalk/netatalk/tree/gh-pages/okf`
- Read raw markdown over HTTPS: `https://netatalk.github.io/netatalk/okf/index.md`
  (the branch carries `.nojekyll`, so files are served as they are)
- Fetch the graph data: `https://raw.githubusercontent.com/Netatalk/netatalk/gh-pages/okf/graph.json`
- Work from a checkout:

```sh
git fetch origin gh-pages
git worktree add ../netatalk-okf gh-pages        # the bundle is ../netatalk-okf/okf
git show origin/gh-pages:okf/index.md            # or read single files without a worktree
```

The [build record](build.md) names
the commit the bundle was built from. Compare it with your checkout before
trusting any line number:

```sh
git merge-base --is-ancestor <build commit> HEAD && echo "checkout is at or after the build"
```

## What is in it

| Path | Content |
|---|---|
| `index.md` | Root index: this guide first, the build record, subsystems, sources, graph data |
| `reading.md` | This page |
| `build.md` | Commit, Doxygen version, the `PREDEFINED` macro set, counts, edge kinds |
| `src/<dir>/<sub>.md` | One concept per subsystem (`src/etc/afpd.md`): files, include and call aggregates, most called functions, functions without a static caller |
| `src/<path>.md` | One concept per source or header file (`src/etc/afpd/fork.c.md`): includes, included by, function tables, functions, types, typedefs and enums, macros, file-scope variables |
| `graph.json` | `nodes` (kind `file`, `function`, `table`) and `edges` (kind `include`, `call`, `use`, `table`, `dispatch`, `doc`); function ids are `path#name`; a `dispatch` edge carries `via`, the `struct::field` it goes through, and a `table` edge into a structure carries `field` |

A concept id is its path without `.md`. Each function is a `###` section in
its file's concept, so `src/etc/afpd/fork.c.md#afp_openfork` is the stable
address of a function. A section carries the signature, the defining lines,
the Doxygen text where there is one, and its edges: calls, called by, calls
through, called through by, uses, dispatched via, mentioned in.

## How to navigate

- Start at `index.md`, open a subsystem concept, then a file concept, then a
  function section. Each index line carries the concept's description, so a
  level can be scanned without opening its files.
- To find a function by name, search the section headings
  (`grep -rl '^### afp_openfork$' okf/src`) or the node ids in `graph.json`.
- To find who depends on a file, read its "Included by" list; for a
  subsystem, read "Headers included by" and "Called from".
- For anything transitive, query `graph.json` rather than following links;
  the recipes below cover the common questions.

## Rules for reading it

1. **Trust an edge that exists; never trust the absence of one.** An include,
   call, table, dispatch, use or doc edge was observed in the source at the
   build commit. A missing edge means Doxygen saw nothing, which is also what
   a call hidden inside a macro, a call through a pointer field no table
   assigns, or code behind an undefined `#ifdef` looks like. Before
   concluding that nothing calls or reads something, search the source tree.
2. **A call edge is a name seen in a body, not a proven runtime call.**
   Doxygen links identifiers by name. The generator resolves a name to a
   definition in the same file first; then, among the functions declared in
   a header the file reaches, to Doxygen's own target, else the only one,
   else the only one in the same directory; then to the only function of
   that name anywhere when the body calls it outside comments and strings;
   and drops the edge otherwise.
   A function whose address is taken inside another body is listed as a
   callee of that body, the registration site, not of the code that later
   invokes the pointer. Read "Calls" as "names" and "Called by" as "named in".
3. **"Functions without a static caller" is a candidate list, never a
   deletion list.** It holds `dlopen`ed UAM entry points, signal handlers,
   callbacks passed as arguments, and functions used only from `test/` or
   platform code outside the scan. Confirm each one with a repository-wide
   search and the build configuration before acting on it.
4. **Table edges name the mechanism, not the index.** "Dispatched via
   postauth_switch" says a function sits in that table. Which AFP command
   reaches it is in the source of
   [switch.c](src/etc/afpd/switch.c.md#postauth_switch),
   not in the edge. A structure-typed table is the exception: its section
   lists the field each function fills, and its `graph.json` edges carry
   `field`.
5. **A dispatch edge is a candidate set.** `setfilowner` calls
   `vol->vfs->vfs_chown`. Doxygen resolves the field, and the generator draws
   one edge to every function any table assigns to it: `RF_chown_adouble`,
   `RF_chown_ea`, `ea_chown` and the `vfs_chown` wrapper in vfs.c. Which one
   runs is decided by the tables selected at run time, here by the volume's
   `appledouble` and `ea` options. "Calls through" names the field; a field
   filled by code rather than by a table initializer has the line but no
   edge.
6. **A mechanical description means "no Doxygen brief", not "nothing to
   know".** A description such as "29 functions, includes 17 project
   headers." is the generator's placeholder. Read the signature and the code;
   the absence of prose says nothing about importance or risk.
7. **Prose is as current as the comment it was copied from.** Doxygen text is
   copied, never checked against the body. Where comment and code disagree,
   the code is the fact and the disagreement is a documentation defect worth
   reporting.
8. **Line numbers belong to the build commit.** "Defined at lines 444 to 883"
   is true for the commit in the build record. On any other commit, locate
   code by function name. For `.l` and `.y` files the lines refer to the view
   Doxygen's input filter produces, not to the file itself.
9. **The graph is a maximal-feature build.** The `PREDEFINED` list in the
   build record turns every feature on and fixes no platform. A function you
   see may not exist in the configuration you are debugging, a platform-only
   branch may be missing, and a macro the list does not define counts as
   undefined even when a system header would define it, which can select a
   file's stub branch over its real code. For "does this run on X" questions,
   read the `#ifdef`s.
10. **Use the graph to choose where to read, then read the source.** The
    bundle is for navigation (index, subsystem, file, function), impact
    candidates (transitive callers), layering (subsystem include and call
    aggregates) and hotspots (most called functions). A claim made to a human
    cites the source file, not this bundle.
11. **Walk anything transitive in `graph.json`; read anything textual in the
    concepts.** Following links across hundreds of files by hand is slow and
    loses edges; a breadth-first search over `graph.json` is one call.
12. **An old `generated.at` is not staleness.** A concept's stamp moves only
    when its extracted content changed; every concept is current for the
    build commit. The bundle's age is the distance between the build commit
    and `main`.
13. **Never edit the bundle.** It is replaced on the next merge. Knowledge
    that should persist goes into a Doxygen comment in the source, which
    appears on the next rebuild, or into a reviewed concept that carries
    `verified`. This page is edited at `doc/developer/okf-knowledge-graph.md`.
14. **Report where the map misled you.** A missing edge you expected, a wrong
    description, a function you needed that had no brief: each is an input to
    the documentation audit and to the generator. Tell the maintainers rather
    than working around it silently.

## Recipes

The recipes read `okf/graph.json` from the current directory; adjust the path
to where the bundle is checked out or downloaded.

Where a function is defined:

```sh
python3 -c 'import json, sys; g = json.load(open("okf/graph.json")); print("\n".join("%s line %s" % (n["id"], n["line"]) for n in g["nodes"] if n["kind"] == "function" and n["id"].endswith("#" + sys.argv[1])))' afp_openfork
```

Transitive callers of a function, through call, table and dispatch edges:

```sh
python3 - <<'EOF'
import collections, json
g = json.load(open("okf/graph.json"))
callers = collections.defaultdict(set)
for e in g["edges"]:
    if e["kind"] in ("call", "table", "dispatch"):
        callers[e["to"]].add(e["from"])
start = "libatalk/adouble/ad_open.c#ad_open"
seen, queue = {start}, [start]
while queue:
    node = queue.pop(0)
    for c in callers[node]:
        if c not in seen:
            seen.add(c)
            queue.append(c)
print("\n".join(sorted(seen - {start})))
EOF
```

Code edges from `libatalk` into `etc`, a layering check (doc edges are
mentions in comments, not dependencies, so they are left out):

```sh
python3 -c 'import json; g = json.load(open("okf/graph.json")); print("\n".join(sorted({e["from"] + " -> " + e["to"] for e in g["edges"] if e["kind"] != "doc" and e["from"].startswith("libatalk/") and e["to"].startswith("etc/")})))'
```

The twenty most called functions:

```sh
python3 -c 'import collections, json; g = json.load(open("okf/graph.json")); c = collections.Counter(e["to"] for e in g["edges"] if e["kind"] == "call"); print("\n".join("%5d %s" % (n, f) for f, n in c.most_common(20)))'
```

## Pointing an agent at the graph

An agent instruction file (`CLAUDE.md`, `AGENTS.md` or similar) needs no more
than this:

> Netatalk publishes a knowledge graph of its sources at
> `https://netatalk.github.io/netatalk/okf/index.md`, also available with
> `git fetch origin gh-pages` under `okf/`. Read `reading.md` there first.
> Use the graph to find callers, dependencies and hotspots before changing
> code, and confirm every conclusion in the source before acting on it.
