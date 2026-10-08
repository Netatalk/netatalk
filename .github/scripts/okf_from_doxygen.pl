#!/usr/bin/env perl

# Generate an Open Knowledge Format (OKF) v0.2 bundle from Doxygen XML.
#
# Runs Doxygen over the C sources with XML output derived from doc/Doxyfile.in,
# then writes one concept per source file and per subsystem. Markdown links
# carry the graph: include edges between files, call edges between functions,
# table edges from function-pointer initializers, dispatch edges from calls
# through function-pointer fields to the functions tables assign to them, and
# documented-reference edges from Doxygen comments. graph.json holds the same
# edges for tooling.
#
# (c) 2026 Andy Lemin (andylemin)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

use strict;
use warnings;
use Cwd          qw(abs_path);
use File::Find   qw(find);
use File::Path   qw(make_path remove_tree);
use File::Temp   qw(tempdir);
use Getopt::Long qw(GetOptions);
use JSON::PP;
use XML::LibXML qw(:libxml);

my $PRODUCER      = 'process:okf-from-doxygen/1';
my $OKF_VERSION   = '0.2';
my @INPUT_DIRS    = qw(bin etc include libatalk sys);
my $READING_GUIDE = 'doc/developer/okf-knowledge-graph.md';
my $PAGES_PREFIX  = 'https://netatalk.github.io/netatalk/okf/';
my %SIMPLESECT = (
                  return    => 'Returns',
                  note      => 'Note',
                  warning   => 'Warning',
                  see       => 'See also',
                  since     => 'Since',
                  remark    => 'Remark',
                  attention => 'Attention',
                  pre       => 'Precondition',
                  post      => 'Postcondition',
                  par       => '',
);
my %PARAMLIST = (param => 'Parameters', retval => 'Return values', exception => 'Exceptions');
my %BLOCK_TAG = map { $_ => 1 }
  qw(parameterlist simplesect itemizedlist orderedlist programlisting verbatim preformatted xrefsect table);
my $DOXYGEN_OVERRIDES = <<'END_OVERRIDES';

GENERATE_HTML          = NO
GENERATE_XML           = YES
XML_PROGRAMLISTING     = NO
REFERENCES_RELATION    = YES
REFERENCED_BY_RELATION = YES
FULL_PATH_NAMES        = YES
FILE_PATTERNS          = *.c *.h *.l *.y
USE_MDFILE_AS_MAINPAGE =
MAX_INITIALIZER_LINES  = 1000
QUIET                  = YES
WARNINGS               = NO
END_OVERRIDES

# A function, variable or struct is identified by its name and the file that
# defines it; the two parts are joined into one hash key.
my $SEP = "\0";

sub key_of    { return join($SEP, @_) }
sub key_parts { return split(/\0/, $_[0], 2) }

# The ($$) prototype makes sort pass the elements in @_, so the comparator works from any package.
sub by_key($$) {
    my ($an, $ah) = key_parts($_[0]);
    my ($bn, $bh) = key_parts($_[1]);
    return $an cmp $bn || $ah cmp $bh;
}

sub yaml_str {
    my ($value) = @_;
    my $text = "$value";
    $text =~ s/\\/\\\\/g;
    $text =~ s/"/\\"/g;
    return "\"$text\"";
}

sub anchor {
    my ($heading) = @_;
    my $slug = lc $heading;
    $slug =~ s/[^a-z0-9 _-]//g;
    $slug =~ tr/ /-/;
    return $slug;
}

sub subsystem_of {
    my ($path) = @_;
    my @parts = split(m{/}, $path);
    return @parts >= 3 ? join('/', @parts[0, 1]) : $parts[0];
}

sub plural {
    my ($count, $noun) = @_;
    return "$count $noun" . ($count == 1 ? '' : 's');
}

sub squeeze {
    my ($text) = @_;
    return join(' ', split(' ', $text));
}

sub trim {
    my ($text) = @_;
    $text =~ s/^\s+//;
    $text =~ s/\s+$//;
    return $text;
}

sub rstrip {
    my ($text) = @_;
    $text =~ s/\s+$//;
    return $text;
}

sub title_case {
    my ($word) = @_;
    return ucfirst(lc($word));
}

sub dir_of {
    my ($path) = @_;
    return $path =~ m{^(.*)/[^/]*$} ? $1 : '';
}

sub base_of {
    my ($path) = @_;
    return $path =~ m{([^/]*)$} ? $1 : $path;
}

sub rel_path {
    my ($target, $start) = @_;
    my @target = split(m{/}, $target);
    my @start  = $start eq '' ? () : split(m{/}, $start);
    while (@target && @start && $target[0] eq $start[0]) {
        shift @target;
        shift @start;
    }
    return join('/', ('..') x @start, @target);
}

sub norm_path {
    my ($path) = @_;
    my @out;
    for my $part (split(m{/}, $path)) {
        next if $part eq '' || $part eq '.';
        if ($part eq '..' && @out && $out[-1] ne '..') {
            pop @out;
        } else {
            push @out, $part;
        }
    }
    return @out ? join('/', @out) : '.';
}

sub read_file {
    my ($path) = @_;
    open(my $fh, '<:encoding(UTF-8)', $path) or die "cannot read $path: $!\n";
    local $/;
    my $text = <$fh>;
    close($fh);
    return $text;
}

sub write_file {
    my ($path, $text) = @_;
    my $dir = dir_of($path);
    make_path($dir) if $dir ne '' && !-d $dir;
    open(my $fh, '>:encoding(UTF-8)', $path) or die "cannot write $path: $!\n";
    print {$fh} $text;
    close($fh);
    return;
}

# --- ElementTree-like accessors over XML::LibXML nodes -------------------

sub et_text {
    my ($node) = @_;
    my $text = '';
    for my $child ($node->childNodes) {
        my $type = $child->nodeType;
        if ($type == XML_TEXT_NODE || $type == XML_CDATA_SECTION_NODE) {
            $text .= $child->data;
        } elsif ($type == XML_ELEMENT_NODE) {
            last;
        }
    }
    return $text;
}

sub first_child {
    my ($node, $name) = @_;
    for my $child ($node->childNodes) {
        return $child if $child->nodeType == XML_ELEMENT_NODE && $child->nodeName eq $name;
    }
    return undef;
}

sub children {
    my ($node, $name) = @_;
    return grep { $_->nodeType == XML_ELEMENT_NODE && $_->nodeName eq $name } $node->childNodes;
}

sub descendants {
    my ($node, $name) = @_;
    return $node->findnodes(".//$name");
}

sub findtext {
    my ($node, $name) = @_;
    my $child = first_child($node, $name);
    return defined $child ? et_text($child) : undef;
}

sub attr {
    my ($node, $name) = @_;
    return defined $node ? $node->getAttribute($name) : undef;
}

sub code_text {
    my ($node) = @_;
    return '' unless defined $node;
    my $text = '';
    for my $child ($node->childNodes) {
        my $type = $child->nodeType;
        if ($type == XML_TEXT_NODE || $type == XML_CDATA_SECTION_NODE) {
            $text .= $child->data;
        } elsif ($type == XML_ELEMENT_NODE) {
            $text .= $child->nodeName eq 'sp' ? ' ' : code_text($child);
        }
    }
    return $text;
}

# --- Doxygen ---------------------------------------------------------------

sub predefined_macros {
    my ($doxyfile_text) = @_;
    my @tokens;
    my $collecting = 0;
    for my $line (split(/\n/, $doxyfile_text)) {
        my $body;
        if (!$collecting) {
            next unless $line =~ /^PREDEFINED\s*=(.*)$/;
            $collecting = 1;
            $body       = $1;
        } else {
            last if $line =~ /^[A-Z_]+\s*=/;
            $body = $line;
        }
        push @tokens, grep { $_ ne '\\' } split(' ', $body);
        last unless rstrip($body) =~ /\\$/;
    }
    return @tokens;
}

sub run_doxygen {
    my ($source, $xml_dir, $version) = @_;
    my $doxyfile_in = read_file("$source/doc/Doxyfile.in");
    my $config      = $doxyfile_in;
    $config =~ s/\@PACKAGE\@/netatalk/g;
    $config =~ s/\@VERSION\@/$version/g;
    $config =~ s/\@ABS_TOP_SRCDIR\@/$source/g;
    my $out_dir = dir_of($xml_dir);
    $config =~ s/\@OUTPUT_DIR\@/$out_dir/g;
    $config =~ s/\@WARN_AS_ERROR\@/NO/g;
    $config .= $DOXYGEN_OVERRIDES;
    $config .= "STRIP_FROM_PATH = $source\n";
    $config .= 'INPUT = ' . join(' ', map {"$source/$_"} @INPUT_DIRS) . "\n";
    my $doxyfile = "$out_dir/Doxyfile.okf";
    write_file($doxyfile, $config);
    my $output = qx(cd "$source/doc" && doxygen "$doxyfile" 2>&1);
    my $status = $? >> 8;

    for my $line (split(/\n/, $output)) {
        print {*STDERR} "$line\n" unless $line =~ /unsupported tag/;
    }
    if ($status != 0) {
        print {*STDERR} "doxygen failed with status $status\n";
        exit 1;
    }
    return predefined_macros($doxyfile_in);
}

# --- Model -----------------------------------------------------------------

sub new_model {
    return {
            files          => {},    # path -> record
            functions      => {},    # key -> record
            structs        => {},    # key -> record
            fields         => {},    # key -> structure member record
            variables      => {},    # key -> record
            refid_key      => {},    # memberdef or compound refid -> [kind, key]
            compound_path  => {},    # file compound refid -> path
            fp_typedefs    => {},    # refid of a typedef that names a function pointer type
            typedef_refids => {},    # typedef refid -> refids its type refers to
            doc_refs       => [],    # [source kind, source key, refid]
    };
}

sub model_file {
    my ($m, $path) = @_;
    $m->{files}{$path} //= {
                            path      => $path,
                            brief     => undef,
                            includes  => [],
                            functions => [],
                            structs   => [],
                            variables => [],
                            defines   => [],
                            typedefs  => [],
                            enums     => [],
    };
    return $m->{files}{$path};
}

sub load {
    my ($xml_dir, $source) = @_;
    my $m = new_model();
    $m->{source} = $source;
    my @docs;
    for my $file (sort glob("$xml_dir/*.xml")) {
        next if base_of($file) eq 'index.xml';
        my $root = XML::LibXML->load_xml(location => $file)->documentElement;
        for my $cd (children($root, 'compounddef')) {
            my $kind = attr($cd, 'kind');
            my $loc  = first_child($cd, 'location');
            if ($kind eq 'file' && defined $loc) {
                my $path = attr($loc, 'file');
                $m->{compound_path}{attr($cd, 'id')} = $path;
                $m->{refid_key}{attr($cd, 'id')}     = ['file', $path];
                my $info = model_file($m, $path);
                $info->{brief}    = first_child($cd, 'briefdescription');
                $info->{compound} = $cd;
                push @docs, [$cd, $path];
            } elsif (($kind eq 'struct' || $kind eq 'union') && defined $loc) {
                my $home = attr($loc, 'file');
                my $name = findtext($cd, 'compoundname');
                my $key  = key_of($name, $home);
                my @members;
                for my $md (descendants($cd, 'memberdef')) {
                    my $type = first_child($md, 'type');
                    my $field = {
                                 key         => key_of("${name}::" . (findtext($md, 'name') // ''), $home),
                                 name        => findtext($md, 'name') // '',
                                 struct      => $key,
                                 type        => trim(code_text($type)),
                                 type_refids =>
                                 [map { attr($_, 'refid') } defined $type ? descendants($type, 'ref') : ()],
                                 args    => findtext($md, 'argsstring') // '',
                                 brief   => first_child($md, 'briefdescription'),
                                 pointer => 0,
                                 callers => {},
                                 tables  => {},
                                 impls   => {},
                    };
                    $m->{fields}{$field->{key}} = $field;
                    $m->{refid_key}{attr($md, 'id')} = ['field', $field->{key}];
                    push @members, $field;
                }
                $m->{structs}{$key} = {
                                       key     => $key,
                                       name    => $name,
                                       home    => $home,
                                       kind    => $kind,
                                       members => \@members,
                                       brief   => first_child($cd, 'briefdescription'),
                                       line    => attr($loc, 'line'),
                };
                $m->{refid_key}{attr($cd, 'id')} = ['struct', $key];
                push @{model_file($m, $home)->{structs}}, $key;
            }
        }
    }
    for my $doc (@docs) {
        my ($cd, $path) = @$doc;
        for my $inc (children($cd, 'includes')) {
            push @{$m->{files}{$path}{includes}},
              [et_text($inc), $m->{compound_path}{attr($inc, 'refid') // ''}, (attr($inc, 'local') // '') eq 'yes'];
        }
        for my $md (descendants($cd, 'memberdef')) {
            my $kind = attr($md, 'kind');
            my $name = findtext($md, 'name') // '';
            my $loc  = first_child($md, 'location');
            my $home = defined $loc ? (attr($loc, 'bodyfile') || attr($loc, 'file')) : $path;
            my $key  = key_of($name, $home);
            if ($kind eq 'function') {
                my $fn = $m->{functions}{$key};
                if (!defined $fn) {
                    $fn = $m->{functions}{$key} = {
                                                   name             => $name,
                                                   home             => $home,
                                                   static           => (attr($md, 'static') // '') eq 'yes',
                                                   definition       => findtext($md, 'definition') // '',
                                                   args             => findtext($md, 'argsstring') // '',
                                                   brief            => first_child($md, 'briefdescription'),
                                                   detailed         => first_child($md, 'detaileddescription'),
                                                   bodystart        => attr($loc, 'bodystart'),
                                                   bodyend          => attr($loc, 'bodyend'),
                                                   declfile         => attr($loc, 'file'),
                                                   declline         => attr($loc, 'line'),
                                                   calls            => [],
                                                   callers          => {},
                                                   uses             => {},
                                                   tables           => {},
                                                   dispatch         => {},
                                                   dispatch_calls   => {},
                                                   dispatch_callers => {},
                                                   doc_in           => {},
                                                   doc_out          => {},
                                                   definitions      => {},
                                                   declared_in      => {$home => 1},
                                                   refids           => {},
                    };
                    push @{model_file($m, $home)->{functions}}, $key;
                }
                $fn->{declared_in}{$path} = 1;
                $fn->{refids}{attr($md, 'id')} = 1;
                if (defined $loc && attr($loc, 'bodystart')) {
                    $fn->{definitions}{attr($loc, 'bodystart')} = 1;
                }
                $m->{refid_key}{attr($md, 'id')} = ['function', $key];
                for my $ref (children($md, 'references')) {
                    push @{$fn->{calls}},
                      [et_text($ref), $m->{compound_path}{attr($ref, 'compoundref') // ''}, attr($ref, 'refid')];
                }
                for my $desc ($fn->{brief}, $fn->{detailed}) {
                    next unless defined $desc;
                    for my $ref (descendants($desc, 'ref')) {
                        push @{$m->{doc_refs}}, ['function', $key, attr($ref, 'refid')];
                    }
                }
            } elsif ($kind eq 'variable') {
                my $init = first_child($md, 'initializer');
                my $type = first_child($md, 'type');
                my $var = $m->{variables}{$key} //= {
                                                     name        => $name,
                                                     home        => $home,
                                                     brief       => first_child($md, 'briefdescription'),
                                                     initializer => defined $init ? code_text($init) : '',
                                                     type_refids => [
                                                                     map { attr($_, 'refid') }
                                                                     defined $type ? descendants($type, 'ref') : ()
                                                     ],
                                                     array       => (findtext($md, 'argsstring') // '') =~ /\[/ ? 1 : 0,
                                                     struct      => undef,
                                                     targets     => {},
                                                     fields      => {},
                                                     line        => attr($loc, 'line'),
                                                     declared_in => {},
                };
                $var->{declared_in}{$path} = 1;
                if (defined $init && $var->{initializer} eq '') {
                    $var->{initializer} = code_text($init);
                }
                $m->{refid_key}{attr($md, 'id')} = ['variable', $key];
                my $vars = model_file($m, $home)->{variables};
                push @$vars, $key unless grep { $_ eq $key } @$vars;
            } elsif ($kind eq 'define') {
                push @{model_file($m, $path)->{defines}}, [$name, first_child($md, 'briefdescription')];
            } elsif ($kind eq 'typedef') {
                my $type = first_child($md, 'type');
                push @{model_file($m, $path)->{typedefs}}, [$name, trim(code_text($type))];
                $m->{fp_typedefs}{attr($md, 'id')} = 1 if (findtext($md, 'argsstring') // '') =~ /^\)\(/;
                $m->{typedef_refids}{attr($md, 'id')} =
                  [map { attr($_, 'refid') } defined $type ? descendants($type, 'ref') : ()];
            } elsif ($kind eq 'enum') {
                my @values = map { findtext($_, 'name') } children($md, 'enumvalue');
                push @{model_file($m, $path)->{enums}}, [$name, \@values];
            }
        }
    }
    # A field is a function pointer when its own declarator says so or its type is a typedef of one.
    for my $field (values %{$m->{fields}}) {
        $field->{pointer} =
          ($field->{args} =~ /^\)\(/ || grep { $m->{fp_typedefs}{$_} } @{$field->{type_refids}}) ? 1 : 0;
    }
    repair_declarations($m);
    merge_declarations($m);
    resolve_edges($m);
    return $m;
}

sub has_text {
    my ($el) = @_;
    return defined $el && $el->textContent =~ /\S/ ? 1 : 0;
}

sub definitions_by_name {
    my ($m) = @_;
    my %definitions;
    for my $key (keys %{$m->{functions}}) {
        push @{$definitions{(key_parts($key))[0]}}, $key if $m->{functions}{$key}{bodystart};
    }
    return \%definitions;
}

# Doxygen pairs a header declaration with the first definition of that name it meets, so papd's
# declaration of a function it copied from afpd points at the afpd body. A declaration in a header
# is re-paired with the one definition of that name in the header's own directory.
sub repair_declarations {
    my ($m) = @_;
    my $definitions = definitions_by_name($m);
    for my $key (sort keys %{$m->{functions}}) {
        my $fn = $m->{functions}{$key};
        for my $decl (sort keys %{$fn->{declared_in}}) {
            next if $decl eq $fn->{home} || dir_of($decl) eq dir_of($fn->{home});
            my @local =
              grep { $_ ne $key && dir_of((key_parts($_))[1]) eq dir_of($decl) } @{$definitions->{$fn->{name}} // []};
            next unless @local == 1;
            my $other = $m->{functions}{$local[0]};
            delete $fn->{declared_in}{$decl};
            $other->{declared_in}{$decl} = 1;
            if ($fn->{declfile} eq $decl) {
                my $line = $fn->{declline};
                ($fn->{declfile}, $fn->{declline}) = ($fn->{home}, $fn->{bodystart});
                ($other->{declfile}, $other->{declline}) = ($decl, $line) if $other->{declfile} eq $other->{home};
            }
        }
    }
    return;
}

# The function body as source text with comments and string literals removed, so that a name
# followed by a parenthesis is a call and not a mention.
sub stripped_body {
    my ($m, $fn) = @_;
    return $fn->{body_text} if exists $fn->{body_text};
    my $text = '';
    my $path = "$m->{source}/$fn->{home}";
    if ($fn->{bodystart} && -f $path) {
        my $lines = $m->{source_lines}{$fn->{home}} //= do {
            open(my $fh, '<:raw', $path) or die "cannot read $path: $!\n";
            my @lines = <$fh>;
            close($fh);
            \@lines;
        };
        my $last = $fn->{bodyend} < @$lines ? $fn->{bodyend} : scalar @$lines;
        $text = join('', @{$lines}[$fn->{bodystart} - 1 .. $last - 1]);
        $text =~ s{("(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*')|/\*.*?\*/|//[^\n]*}{defined $1 ? '""' : ' '}gse;
    }
    return $fn->{body_text} = $text;
}

sub body_calls {
    my ($m, $fn, $name) = @_;
    return stripped_body($m, $fn) =~ /(?<![A-Za-z0-9_])\Q$name\E\s*\(/ ? 1 : 0;
}

# A structure field is called when it follows a member access and is applied, as
# vol->vfs->vfs_chown(...) or (*ad->ad_ops->ad_mkrf)(...); an assignment or test of the field is not a call.
sub body_calls_field {
    my ($m, $fn, $name) = @_;
    return stripped_body($m, $fn) =~ /(?:->|\.)\s*\Q$name\E\s*\)?\s*\(/ ? 1 : 0;
}

# Fold a declaration Doxygen did not pair with its definition into the one definition of that name.
sub merge_declarations {
    my ($m) = @_;
    my %definitions;
    for my $key (keys %{$m->{functions}}) {
        push @{$definitions{(key_parts($key))[0]}}, $key if $m->{functions}{$key}{bodystart};
    }
    my %remap;
    for my $key (sort grep { !$m->{functions}{$_}{bodystart} } keys %{$m->{functions}}) {
        my $defs = $definitions{(key_parts($key))[0]} // [];
        next unless @$defs == 1;
        my $stub       = delete $m->{functions}{$key};
        my $target_key = $defs->[0];
        my $target     = $m->{functions}{$target_key};
        $target->{declared_in}{$_} = 1 for keys %{$stub->{declared_in}};
        if ($target->{declfile} eq $target->{home}) {
            ($target->{declfile}, $target->{declline}) = ($stub->{declfile}, $stub->{declline});
        }
        $target->{brief}    = $stub->{brief}    if !has_text($target->{brief})    && has_text($stub->{brief});
        $target->{detailed} = $stub->{detailed} if !has_text($target->{detailed}) && has_text($stub->{detailed});
        $m->{refid_key}{$_} = ['function', $target_key] for keys %{$stub->{refids}};
        my $list = $m->{files}{$stub->{home}}{functions};
        @$list = grep { $_ ne $key } @$list;
        $remap{$key} = $target_key;
    }
    $_->[1] = $remap{$_->[1]} // $_->[1] for @{$m->{doc_refs}};
    return;
}

# Files reachable from each file through project includes, the file itself included.
sub include_closure {
    my ($m) = @_;
    my %closure;
    my $reach;
    $reach = sub {
        my ($path) = @_;
        return $closure{$path} if exists $closure{$path};
        $closure{$path} = {$path => 1};
        for my $inc (@{$m->{files}{$path}{includes}}) {
            my $target = $inc->[1];
            next unless defined $target && exists $m->{files}{$target};
            my $reached = $reach->($target);
            $closure{$path}{$_} = 1 for keys %$reached;
        }
        return $closure{$path};
    };
    $reach->($_) for keys %{$m->{files}};
    return \%closure;
}

sub intersects {
    my ($a_set, $b_set) = @_;
    for my $item (keys %$a_set) {
        return 1 if exists $b_set->{$item};
    }
    return 0;
}

# The structure a table variable is an instance of, found through typedefs; arrays are not mapped.
sub table_struct {
    my ($m, $var) = @_;
    return undef if $var->{array};
    my @queue = @{$var->{type_refids}};
    my %seen;
    while (defined(my $refid = shift @queue)) {
        next if $seen{$refid}++;
        my $target = $m->{refid_key}{$refid};
        return $m->{structs}{$target->[1]} if defined $target && $target->[0] eq 'struct';
        push @queue, @{$m->{typedef_refids}{$refid} // []};
    }
    return undef;
}

# The top-level entries of a brace initializer, or undef when the text is not one brace group.
sub initializer_entries {
    my ($text) = @_;
    $text =~ s/^\s*=\s*//;
    return undef unless $text =~ /\A\{(.*)\}\s*\z/s;
    my $inner = $1;
    my @entries;
    my ($depth, $current) = (0, '');
    while ($inner =~ /\G("(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|[(\[{]|[)\]}]|,|[^,()\[\]{}"']+|["'])/gcs) {
        my $piece = $1;
        if ($piece eq ',' && $depth == 0) {
            push @entries, $current;
            $current = '';
            next;
        }
        $depth++ if $piece =~ /^[(\[{]$/;
        $depth-- if $piece =~ /^[)\]}]$/;
        $current .= $piece;
    }
    push @entries, $current;
    return [map { trim($_) } grep {/\S/} @entries];
}

sub resolve_edges {
    my ($m) = @_;

    # Doxygen matches identifiers in a body, comments included, against every known member of
    # that name, so a parameter called zone or a login() mentioned in a comment is reported as
    # a reference to an unrelated function or global, and a call may be attributed to a
    # same-named static or macro in another file. A name is resolved the way C scoping would:
    # a definition in the referencing file first; among the definitions declared in a header
    # the file reaches, Doxygen's own target, else the only one, else the only one in the same
    # directory; finally the only definition of that name anywhere, provided the body calls it
    # outside comments and strings. Variables need a visible declaration. A reference to a
    # function-pointer field of a structure, which Doxygen resolves through the variable's type,
    # becomes a dispatch edge to every function a table initializer assigns to that field.
    my %by_name;
    for my $key (keys %{$m->{functions}}) {
        push @{$by_name{(key_parts($key))[0]}}, $key;
    }
    my $closure = include_closure($m);
    my $pick    = sub {
        my ($name, $attributed, $caller, $home, $visible) = @_;
        my $local = key_of($name, $home);
        return $local if exists $m->{functions}{$local};
        my @candidates = @{$by_name{$name} // []};
        my @seen       = grep { intersects($m->{functions}{$_}{declared_in}, $visible) } @candidates;
        return $attributed if defined $attributed && grep { $_ eq $attributed } @seen;
        return $seen[0] if @seen == 1;
        my $dir  = dir_of($home);
        my @near = grep { dir_of((key_parts($_))[1]) eq $dir } @seen;
        return $near[0]       if @near == 1;
        return $candidates[0] if @candidates == 1 && defined $caller && body_calls($m, $caller, $name);
        return undef;
    };
    for my $key (keys %{$m->{functions}}) {
        my $fn      = $m->{functions}{$key};
        my $visible = $closure->{$fn->{home}};
        my %resolved;
        for my $call (@{$fn->{calls}}) {
            my ($name, $target_path, $refid) = @$call;
            my $target = $m->{refid_key}{$refid // ''};
            if (defined $target && $target->[0] eq 'variable') {
                if (intersects($m->{variables}{$target->[1]}{declared_in}, $visible)) {
                    $fn->{uses}{$target->[1]} = 1;
                }
                next;
            }
            if (defined $target && $target->[0] eq 'field') {
                my $field = $m->{fields}{$target->[1]};
                if ($field->{pointer} && body_calls_field($m, $fn, $field->{name})) {
                    $fn->{dispatch}{$field->{key}} = 1;
                    $field->{callers}{$key} = 1;
                }
                next;
            }
            my $attributed =
                defined $target && $target->[0] eq 'function' ? $target->[1]
              : defined $target_path                          ? key_of($name, $target_path)
              :                                                 undef;
            my $callee = $pick->($name, $attributed, $fn, $fn->{home}, $visible);
            $resolved{$callee} = 1 if defined $callee;
        }
        delete $resolved{$key};
        $fn->{calls} = \%resolved;
        for my $callee (keys %resolved) {
            $m->{functions}{$callee}{callers}{$key} = 1;
        }
    }
    # A structure-typed table is read entry by entry, designated or positional, so each function
    # is tied to the field it fills. Other initializers are scanned for names, with designators
    # dropped so a field name is not read as a same-named function.
    for my $vkey (keys %{$m->{variables}}) {
        my $var = $m->{variables}{$vkey};
        next unless index($var->{initializer}, '{') >= 0;
        my $visible = $closure->{$var->{home}};
        my $struct  = table_struct($m, $var);
        my $entries = defined $struct ? initializer_entries($var->{initializer}) : undef;
        my @values;
        if (defined $entries) {
            $var->{struct} = $struct->{key};
            my $members  = $struct->{members};
            my $position = 0;
            for my $entry (@$entries) {
                if ($entry =~ /^\.([A-Za-z_][A-Za-z0-9_]*)\s*=(?!=)\s*(.*)$/s) {
                    my $designated = $1;
                    $entry = $2;
                    ($position) = grep { $members->[$_]{name} eq $designated } 0 .. $#$members;
                    $position //= scalar @$members;
                } elsif ($entry =~ /^\.[A-Za-z_][A-Za-z0-9_.]*\s*=(?!=)\s*(.*)$/s) {
                    $entry    = $1;
                    $position = scalar @$members;
                }
                push @values, [$position < @$members ? $members->[$position] : undef, $entry];
                $position++;
            }
        } else {
            (my $text = $var->{initializer}) =~ s/\.\s*[A-Za-z_][A-Za-z0-9_.]*\s*=(?!=)/ /g;
            @values = ([undef, $text]);
        }
        for my $value (@values) {
            my ($field, $text) = @$value;
            my %idents = map { $_ => 1 } ($text =~ /([A-Za-z_][A-Za-z0-9_]*)/g);
            for my $ident (sort keys %idents) {
                my $callee = $pick->($ident, undef, undef, $var->{home}, $visible);
                next unless defined $callee;
                $var->{targets}{$callee} = 1;
                $m->{functions}{$callee}{tables}{$vkey} = 1;
                next unless defined $field && $field->{pointer};
                $var->{fields}{$callee}         = $field->{key};
                $field->{impls}{$callee}{$vkey} = 1;
                $field->{tables}{$vkey}         = 1;
            }
        }
    }
    for my $key (keys %{$m->{functions}}) {
        my $fn = $m->{functions}{$key};
        for my $fkey (keys %{$fn->{dispatch}}) {
            for my $impl (keys %{$m->{fields}{$fkey}{impls}}) {
                next if $impl eq $key;
                $fn->{dispatch_calls}{$impl}{$fkey} = 1;
                $m->{functions}{$impl}{dispatch_callers}{$key}{$fkey} = 1;
            }
        }
    }
    for my $doc_ref (@{$m->{doc_refs}}) {
        my ($kind, $src_key, $refid) = @$doc_ref;
        my $target = $m->{refid_key}{$refid // ''};
        if (defined $target && $target->[0] eq 'function' && $target->[1] ne $src_key) {
            $m->{functions}{$src_key}{doc_out}{$target->[1]} = 1;
            $m->{functions}{$target->[1]}{doc_in}{$src_key} = 1;
        }
    }
    return;
}

# --- Writer ----------------------------------------------------------------

package Writer;

sub new {
    my ($class, %args) = @_;
    my $self = {
                m            => $args{model},
                out          => $args{out},
                repo_url     => $args{repo_url},
                branch       => $args{branch},
                commit       => $args{commit},
                commit_date  => $args{commit_date},
                previous     => $args{previous},
                cur          => undef,
                descriptions => {},
    };
    return bless $self, $class;
}

sub file_concept      { return "src/$_[1].md" }
sub subsystem_concept { return "src/$_[1].md" }

sub rel {
    my ($self, $target, $fragment) = @_;
    my $link = main::rel_path($target, main::dir_of($self->{cur}));
    return $link . (defined $fragment && $fragment ne '' ? "#$fragment" : '');
}

sub link_function {
    my ($self, $key)  = @_;
    my ($name, $home) = main::key_parts($key);
    return $self->rel($self->file_concept($home), main::anchor($name));
}

sub link_refid {
    my ($self, $refid) = @_;
    my $target = $self->{m}{refid_key}{$refid // ''};
    return undef unless defined $target;
    my ($kind, $key) = @$target;
    return $self->rel($self->file_concept($key)) if $kind eq 'file';
    return $self->link_function($key)            if $kind eq 'function';
    my ($name, $home) = main::key_parts($key);
    if ($kind eq 'variable') {
        my $has_targets = %{$self->{m}{variables}{$key}{targets}} ? 1 : 0;
        return $self->rel($self->file_concept($home), $has_targets ? main::anchor($name) : undef);
    }
    return $self->link_struct($self->{m}{structs}{$key})                             if $kind eq 'struct';
    return $self->link_struct($self->{m}{structs}{$self->{m}{fields}{$key}{struct}}) if $kind eq 'field';
    return undef;
}

# --- Doxygen description rendering ---

sub inline {
    my ($self, $el) = @_;
    my $text = '';
    for my $child ($el->childNodes) {
        my $type = $child->nodeType;
        if ($type == XML::LibXML::XML_TEXT_NODE || $type == XML::LibXML::XML_CDATA_SECTION_NODE) {
            $text .= $child->data;
            next;
        }
        next unless $type == XML::LibXML::XML_ELEMENT_NODE;
        my $tag = $child->nodeName;
        if ($tag eq 'ref') {
            my $label  = main::trim(main::et_text($child));
            my $target = $self->link_refid($child->getAttribute('refid'));
            $text .= defined $target ? "[$label]($target)" : $label;
        } elsif ($tag eq 'computeroutput' || $tag eq 'verbatim') {
            $text .= '`' . main::trim(main::code_text($child)) . '`';
        } elsif ($tag eq 'bold') {
            $text .= '**' . $self->inline($child) . '**';
        } elsif ($tag eq 'emphasis') {
            $text .= '*' . $self->inline($child) . '*';
        } elsif ($BLOCK_TAG{$tag}) {
            next;
        } elsif ($tag eq 'linebreak') {
            $text .= ' ';
        } else {
            $text .= $self->inline($child);
        }
    }
    return main::squeeze($text);
}

sub blocks {
    my ($self, $desc) = @_;
    my @out;
    return @out unless defined $desc;
    for my $para (main::children($desc, 'para')) {
        my $text = $self->inline($para);
        push @out, $text if $text ne '';
        for my $child (grep { $_->nodeType == XML::LibXML::XML_ELEMENT_NODE } $para->childNodes) {
            my $tag = $child->nodeName;
            if ($tag eq 'parameterlist') {
                my @items;
                for my $item (main::children($child, 'parameteritem')) {
                    my $names =
                      join(
                           ', ',
                           map { '`' . main::trim(main::et_text($_)) . '`' } main::descendants($item, 'parametername')
                      );
                    my $body = join(' ', $self->blocks(main::first_child($item, 'parameterdescription')));
                    push @items, $body ne '' ? "* $names: $body" : "* $names";
                }
                my $kind = $child->getAttribute('kind') // '';
                push @out, ($PARAMLIST{$kind} // main::title_case($kind)) . ":\n" . join("\n", @items);
            } elsif ($tag eq 'simplesect') {
                my $kind  = $child->getAttribute('kind') // '';
                my $label = exists $SIMPLESECT{$kind} ? $SIMPLESECT{$kind} : main::title_case($kind);
                my $body  = join(' ', $self->blocks($child));
                push @out, $label ne '' ? "$label: $body" : $body;
            } elsif ($tag eq 'itemizedlist' || $tag eq 'orderedlist') {
                my $marker = $tag eq 'itemizedlist' ? '* ' : '1. ';
                push @out,
                  join("\n", map { $marker . join(' ', $self->blocks($_)) } main::children($child, 'listitem'));
            } elsif ($tag eq 'programlisting') {
                my $code = join("\n", map { main::code_text($_) } main::children($child, 'codeline'));
                push @out, "```\n" . main::rstrip($code) . "\n```";
            } elsif ($tag eq 'verbatim' || $tag eq 'preformatted') {
                push @out, "```\n" . main::rstrip(main::code_text($child)) . "\n```";
            } elsif ($tag eq 'xrefsect') {
                push @out,
                  (main::findtext($child, 'xreftitle') // '') . ': '
                  . join(' ', $self->blocks(main::first_child($child, 'xrefdescription')));
            }
        }
    }
    return @out;
}

sub brief {
    my ($self, $el) = @_;
    return join(' ', $self->blocks($el));
}

# Brief description as link-free text for frontmatter and index lines.
sub plain {
    my ($self, $el) = @_;
    return '' unless defined $el;
    return main::squeeze($el->textContent);
}

# --- concepts ---

sub frontmatter {
    my ($self, $type, $title, $description, $tags, $resource) = @_;
    my @lines =
      ('---', "type: $type", 'title: ' . main::yaml_str($title), 'description: ' . main::yaml_str($description));
    push @lines, 'resource: ' . main::yaml_str($resource) if defined $resource;
    push @lines, 'tags: [' . join(', ', map { main::yaml_str($_) } @$tags) . ']';
    push @lines, 'status: stable';
    push @lines, "generated: { by: $PRODUCER, at: __GENERATED_AT__ }";
    push @lines, '---';
    return @lines;
}

sub heading {
    my ($self, $level, $text) = @_;
    return ('#' x $level) . " $text";
}

sub write_concept {
    my ($self, $concept, $lines, $description) = @_;
    my $text  = main::rstrip(join("\n", @$lines)) . "\n";
    my $stamp = $self->{commit_date};
    if (defined $self->{previous}) {
        my $old = "$self->{previous}/$concept";
        if (-f $old) {
            my $old_text = main::read_file($old);
            if ($old_text =~ /^generated: \{ by: .*, at: (.*) \}$/m) {
                my $old_stamp = $1;
                (my $old_norm = $old_text) =~ s/^generated: \{ by: .*, at: .* \}$/generated: __X__/mg;
                (my $new_norm = $text) =~ s/\Qgenerated: { by: $PRODUCER, at: __GENERATED_AT__ }\E/generated: __X__/g;
                $stamp = $old_stamp if $old_norm eq $new_norm;
            }
        }
    }
    $text =~ s/__GENERATED_AT__/$stamp/g;
    main::write_file("$self->{out}/$concept", $text);
    $self->{descriptions}{$concept} = $description;
    return;
}

sub links_to {
    my ($self, @keys) = @_;
    return join(', ', map { '[' . (main::key_parts($_))[0] . '](' . $self->link_function($_) . ')' } @keys);
}

sub links_to_tables {
    my ($self, @keys) = @_;
    return
      join(
           ', ',
           map {
               my ($name, $home) = main::key_parts($_);
               "[$name](" . $self->rel($self->file_concept($home), main::anchor($name)) . ')'
           } @keys
      );
}

sub link_struct {
    my ($self, $st) = @_;
    return $self->rel($self->file_concept($st->{home}), main::anchor("$st->{kind} $st->{name}"));
}

sub link_field {
    my ($self, $fkey) = @_;
    my $field = $self->{m}{fields}{$fkey};
    my $st    = $self->{m}{structs}{$field->{struct}};
    return "[`$st->{name}::$field->{name}`](" . $self->link_struct($st) . ')';
}

sub function_section {
    my ($self, $key, $lines) = @_;
    my $fn = $self->{m}{functions}{$key};
    push @$lines, '', $self->heading(3, $fn->{name}), '', '```c';
    my $static = $fn->{static} && $fn->{definition} !~ /^static/ ? 'static ' : '';
    push @$lines, "$static$fn->{definition}$fn->{args}", '```', '';
    my @where;
    if ($fn->{bodystart}) {
        push @where, "Defined at lines $fn->{bodystart} to $fn->{bodyend}.";
    } elsif ($fn->{declfile}) {
        push @where, "Declared at $fn->{declfile} line $fn->{declline}; no definition in the scanned sources.";
    }
    my $definitions = scalar keys %{$fn->{definitions}};
    push @where, "Defined $definitions times under conditional compilation." if $definitions > 1;
    if ($fn->{declfile} && $fn->{bodystart} && $fn->{declfile} ne $fn->{home}) {
        push @where, "Declared in [$fn->{declfile}](" . $self->rel($self->file_concept($fn->{declfile})) . ').';
    }
    push @$lines, join(' ', @where);
    my $brief = $self->brief($fn->{brief});
    push @$lines, '', $brief if $brief ne '';
    push @$lines, '', $_ for $self->blocks($fn->{detailed});
    for my $pair (['Calls', $fn->{calls}], ['Called by', $fn->{callers}]) {
        my ($label, $keys) = @$pair;
        push @$lines, '', "$label: " . $self->links_to(sort main::by_key keys %$keys) if %$keys;
    }
    for my $fkey (sort main::by_key keys %{$fn->{dispatch}}) {
        my @impls = sort main::by_key grep { $_ ne $key } keys %{$self->{m}{fields}{$fkey}{impls}};
        push @$lines, '',
          'Calls through '
          . $self->link_field($fkey) . ': '
          . (@impls ? $self->links_to(@impls) : 'no table assigns this field');
    }
    my %through;
    for my $caller (keys %{$fn->{dispatch_callers}}) {
        $through{$_}{$caller} = 1 for keys %{$fn->{dispatch_callers}{$caller}};
    }
    for my $fkey (sort main::by_key keys %through) {
        push @$lines, '',
          'Called through '
          . $self->link_field($fkey) . ' by: '
          . $self->links_to(sort main::by_key keys %{$through{$fkey}});
    }
    if (%{$fn->{uses}}) {
        push @$lines, '',
          'Uses file-scope variables: '
          . join(
                 ', ',
                 map {
                     my ($name, $home) = main::key_parts($_);
                     $home ne $fn->{home}
                     ? "`$name` in [$home](" . $self->rel($self->file_concept($home)) . ')'
                     : "`$name`"
                 } sort main::by_key keys %{$fn->{uses}}
          );
    }
    if (%{$fn->{tables}}) {
        push @$lines, '', 'Dispatched via: ' . $self->links_to_tables(sort main::by_key keys %{$fn->{tables}});
    }
    if (%{$fn->{doc_in}}) {
        push @$lines, '',
          'Mentioned in the documentation of: ' . $self->links_to(sort main::by_key keys %{$fn->{doc_in}});
    }
    return;
}

sub file_description {
    my ($self, $info) = @_;
    my $brief = $self->plain($info->{brief});
    return $brief if $brief ne '';
    my @parts;
    push @parts, main::plural(scalar @{$info->{functions}}, 'function') if @{$info->{functions}};
    push @parts, main::plural(scalar @{$info->{structs}},   'type')     if @{$info->{structs}};
    my $project_includes = grep { defined $_->[1] } @{$info->{includes}};
    push @parts, 'includes ' . main::plural($project_includes, 'project header') if $project_includes;
    return (@parts ? join(', ', @parts) : 'No functions or types') . '.';
}

sub write_file {
    my ($self, $path) = @_;
    my $m       = $self->{m};
    my $info    = $m->{files}{$path};
    my $concept = $self->file_concept($path);
    $self->{cur} = $concept;
    my $type        = $path =~ /\.h$/ ? 'C Header File' : 'C Source File';
    my $description = $self->file_description($info);
    my $subsystem   = main::subsystem_of($path);
    my @lines =
      $self->frontmatter($type, $path, $description, [$subsystem], "$self->{repo_url}/blob/$self->{branch}/$path");
    push @lines, '',
      "Part of the [$subsystem]("
      . $self->rel($self->subsystem_concept($subsystem))
      . ') subsystem. Built from the commit recorded in [build]('
      . $self->rel('build.md') . ').';

    if ($path =~ /\.[ly]$/) {
        push @lines, '',
          'Line numbers refer to the view produced by the Doxygen input filter for this file type, '
          . 'not to the file itself.';
    }
    my @project = sort { $a->[0] cmp $b->[0] || $a->[1] cmp $b->[1] }
      map { [$_->[0], $_->[1]] } grep { defined $_->[1] } @{$info->{includes}};
    my @system = sort map { $_->[0] } grep { !defined $_->[1] } @{$info->{includes}};

    if (@project || @system) {
        push @lines, '', $self->heading(1, 'Includes'), '';
        push @lines, "* [$_->[0]](" . $self->rel($self->file_concept($_->[1])) . ')' for @project;
        push @lines, '* System headers: ' . join(', ', map {"`$_`"} @system) if @system;
    }
    my @included_by = sort grep {
        my $other = $m->{files}{$_};
        grep { defined $_->[1] && $_->[1] eq $path } @{$other->{includes}};
    } keys %{$m->{files}};
    if (@included_by) {
        push @lines, '', $self->heading(1, 'Included by'), '';
        push @lines, "* [$_](" . $self->rel($self->file_concept($_)) . ')' for @included_by;
    }
    my @tables = grep { %{$m->{variables}{$_}{targets}} } @{$info->{variables}};
    if (@tables) {
        push @lines, '', $self->heading(1, 'Function tables');
        for my $vkey (sort main::by_key @tables) {
            my $var = $m->{variables}{$vkey};
            push @lines, '', $self->heading(3, $var->{name}), '';
            my $brief = $self->brief($var->{brief});
            push @lines, $brief, '' if $brief ne '';
            if (defined $var->{struct}) {
                my $st = $m->{structs}{$var->{struct}};
                my %by_field;
                push @{$by_field{$var->{fields}{$_} // ''}}, $_ for keys %{$var->{targets}};
                push @lines, "Initialized at line $var->{line} as `$st->{kind} $st->{name}`. Assigns:", '';
                for my $fkey (sort main::by_key grep { $_ ne '' } keys %by_field) {
                    push @lines,
                      "* `$m->{fields}{$fkey}{name}`: " . $self->links_to(sort main::by_key @{$by_field{$fkey}});
                }
                push @lines, '* Not tied to a field: ' . $self->links_to(sort main::by_key @{$by_field{''}})
                  if exists $by_field{''};
            } else {
                push @lines, "Initialized at line $var->{line}. Dispatches to: "
                  . $self->links_to(sort main::by_key keys %{$var->{targets}});
            }
        }
    }
    if (@{$info->{functions}}) {
        push @lines, '', $self->heading(1, 'Functions');
        my @ordered = sort {
            my $fa = $m->{functions}{$a};
            my $fb = $m->{functions}{$b};
            ($fa->{bodystart} || $fa->{declline} || 0) <=> ($fb->{bodystart} || $fb->{declline} || 0)
              || $fa->{name} cmp $fb->{name}
        } @{$info->{functions}};
        $self->function_section($_, \@lines) for @ordered;
    }
    if (@{$info->{structs}}) {
        push @lines, '', $self->heading(1, 'Types');
        for my $key (sort main::by_key @{$info->{structs}}) {
            my $st = $m->{structs}{$key};
            push @lines, '', $self->heading(3, "$st->{kind} $st->{name}"), '';
            my $brief = $self->brief($st->{brief});
            push @lines, $brief, '' if $brief ne '';
            push @lines, "Defined at line $st->{line}.";
            for my $member (@{$st->{members}}) {
                my $mb   = $self->brief($member->{brief});
                my $line = "* `$member->{type} $member->{name}`" . ($mb ne '' ? ": $mb" : '');
                my @notes;
                push @notes, 'assigned in ' . $self->links_to_tables(sort main::by_key keys %{$member->{tables}})
                  if %{$member->{tables}};
                push @notes, 'called through by ' . $self->links_to(sort main::by_key keys %{$member->{callers}})
                  if %{$member->{callers}};
                $line .= ($mb ne '' ? ' ' : ': ') . ucfirst(join('; ', @notes)) . '.' if @notes;
                push @lines, $line;
            }
        }
    }
    if (@{$info->{typedefs}} || @{$info->{enums}}) {
        push @lines, '', $self->heading(1, 'Typedefs and enums'), '';
        for my $typedef (sort { $a->[0] cmp $b->[0] || $a->[1] cmp $b->[1] } @{$info->{typedefs}}) {
            push @lines, "* `typedef $typedef->[1] $typedef->[0]`";
        }
        for
          my $enum (sort { $a->[0] cmp $b->[0] || join("\0", @{$a->[1]}) cmp join("\0", @{$b->[1]}) } @{$info->{enums}})
        {
            my ($name, $values) = @$enum;
            push @lines, '* `enum ' . ($name ne '' ? $name : '(anonymous)') . '`: ' . join(', ', map {"`$_`"} @$values);
        }
    }
    if (@{$info->{defines}}) {
        push @lines, '', $self->heading(1, 'Macros'), '';
        my @documented = sort { $a->[0] cmp $b->[0] || $a->[1] cmp $b->[1] }
          grep { $_->[1] ne '' } map { [$_->[0], $self->brief($_->[1])] } @{$info->{defines}};
        my @plain = sort map { $_->[0] } grep { $self->brief($_->[1]) eq '' } @{$info->{defines}};
        push @lines, "* `$_->[0]`: $_->[1]" for @documented;
        push @lines, '* Undocumented: ' . join(', ', map {"`$_`"} @plain) if @plain;
    }
    my @globals = sort map { (main::key_parts($_))[0] } grep { !%{$m->{variables}{$_}{targets}} } @{$info->{variables}};
    if (@globals) {
        push @lines, '', $self->heading(1, 'File-scope variables'), '', join(', ', map {"`$_`"} @globals);
    }
    $self->write_concept($concept, \@lines, $description);
    return;
}

sub write_subsystem {
    my ($self, $name, $paths) = @_;
    my $m       = $self->{m};
    my $concept = $self->subsystem_concept($name);
    $self->{cur} = $concept;
    my @functions   = map { @{$m->{files}{$_}{functions}} } @$paths;
    my $description = main::plural(scalar @$paths, 'file') . ', ' . main::plural(scalar @functions, 'function') . '.';
    my @lines =
      $self->frontmatter('Subsystem', $name, $description, [$name], "$self->{repo_url}/tree/$self->{branch}/$name");
    push @lines, '', $self->heading(1, 'Files'), '';

    for my $path (sort @$paths) {
        push @lines,
          "* [$path]("
          . $self->rel($self->file_concept($path)) . '): '
          . $self->{descriptions}{$self->file_concept($path)};
    }
    my (%depends, %used_by, %calls_out, %calls_in);
    for my $path (@$paths) {
        for my $inc (@{$m->{files}{$path}{includes}}) {
            next unless defined $inc->[1];
            my $other = main::subsystem_of($inc->[1]);
            $depends{$other}++ if $other ne $name;
        }
    }
    for my $other_path (keys %{$m->{files}}) {
        my $other = main::subsystem_of($other_path);
        next if $other eq $name;
        for my $inc (@{$m->{files}{$other_path}{includes}}) {
            $used_by{$other}++ if defined $inc->[1] && main::subsystem_of($inc->[1]) eq $name;
        }
    }
    for my $key (@functions) {
        for my $callee (keys %{$m->{functions}{$key}{calls}}) {
            my $other = main::subsystem_of((main::key_parts($callee))[1]);
            $calls_out{$other}++ if $other ne $name;
        }
        for my $caller (keys %{$m->{functions}{$key}{callers}}) {
            my $other = main::subsystem_of((main::key_parts($caller))[1]);
            $calls_in{$other}++ if $other ne $name;
        }
    }
    for my $section (
                     ['Includes headers from', \%depends,   'includes'], ['Headers included by', \%used_by, 'includes'],
                     ['Calls into',            \%calls_out, 'calls'],    ['Called from',         \%calls_in, 'calls']
    ) {
        my ($title, $counts, $unit) = @$section;
        next unless %$counts;
        push @lines, '', $self->heading(1, $title), '';
        for my $other (sort { $counts->{$b} <=> $counts->{$a} || $a cmp $b } keys %$counts) {
            push @lines, "* [$other](" . $self->rel($self->subsystem_concept($other)) . "): $counts->{$other} $unit";
        }
    }
    my @ranked = sort {
        scalar(keys %{$m->{functions}{$b}{callers}}) <=> scalar(keys %{$m->{functions}{$a}{callers}})
          || main::by_key($a, $b)
    } @functions;
    @ranked = grep { %{$m->{functions}{$_}{callers}} } @ranked[0 .. ($#ranked < 9 ? $#ranked : 9)];
    if (@ranked) {
        push @lines, '', $self->heading(1, 'Most called functions'), '';
        for my $key (@ranked) {
            push @lines,
              '* ['
              . (main::key_parts($key))[0] . ']('
              . $self->link_function($key) . '): '
              . scalar(keys %{$m->{functions}{$key}{callers}})
              . ' callers';
        }
    }
    my @unreferenced = sort main::by_key grep {
        my $fn = $m->{functions}{$_};
        $fn->{bodystart} && !%{$fn->{callers}} && !%{$fn->{tables}} && !%{$fn->{doc_in}} && $fn->{name} ne 'main'
    } @functions;
    if (@unreferenced) {
        push @lines, '', $self->heading(1, 'Functions without a static caller'), '',
          'No call, table or documentation edge reaches these functions in the scanned sources. '
          . 'Entry points reached through dlopen, signals, callbacks passed as arguments, or code outside the scanned directories appear here too.',
          '', $self->links_to(@unreferenced);
    }
    $self->write_concept($concept, \@lines, $description);
    return;
}

sub write_build {
    my ($self, $doxygen_version, $predefined, $stats) = @_;
    my $concept = 'build.md';
    $self->{cur} = $concept;
    my $description =
      'Netatalk commit ' . substr($self->{commit}, 0, 12) . " rendered by $PRODUCER from Doxygen $doxygen_version.";
    my @lines = $self->frontmatter('Build Record', 'Build record', $description, ['build'],
                                   "$self->{repo_url}/commit/$self->{commit}");
    push @lines, '', $self->heading(1, 'Source'), '',
      "* Commit: [$self->{commit}]($self->{repo_url}/commit/$self->{commit}), committed $self->{commit_date}",
      '* Scanned directories: ' . join(', ', map {"`$_`"} @INPUT_DIRS),
      "* Doxygen: $doxygen_version, configured from `doc/Doxyfile.in` with XML output and reference relations",
      '', $self->heading(1, 'Preprocessor view'), '',
'Conditional code is included as if these macros were defined, so the graph reflects a maximal feature build rather than any one platform:',
      '', join(', ', map {"`$_`"} @$predefined),
      '', $self->heading(1, 'Counts'), '';
    push @lines, "* $_->[0]: $_->[1]" for @$stats;
    push @lines, '', $self->heading(1, 'Edge kinds'), '',
      '* include: a file includes a project header',
      '* call: a function body references another function',
'* table: a variable initializer names a function (dispatch and operation tables); for a structure-typed table the edge carries the field',
'* dispatch: a function body calls through a function-pointer field of a structure; one edge per function a table assigns to that field',
      '* use: a function body references a file-scope variable',
      '* doc: a Doxygen comment references a function',
      '',
      'Doxygen records call edges from function bodies by name. A function whose address is taken inside another body '
      . 'appears as a callee of that body, which is the registration site rather than the eventual caller. '
      . 'A call through a function-pointer field reaches every function any table assigns to the field, so dispatch '
      . 'edges are candidates; a field filled by code rather than by a table initializer has no dispatch edge.';
    $self->write_concept($concept, \@lines, $description);
    return;
}

# The developer page, with links into the published bundle made bundle-relative.
sub write_reading {
    my ($self, $guide) = @_;
    my $concept = 'reading.md';
    $self->{cur} = $concept;
    my $description = 'Where the graph is, how to navigate it, the rules for reading it (trust an edge that '
      . 'exists, never the absence of one) and recipes over graph.json.';
    my @lines = $self->frontmatter('Reading Guide', 'OKF Knowledge Graph', $description, ['guide']);
    my $text  = main::read_file($guide);
    $text =~ s/\]\(\Q$PAGES_PREFIX\E([^)]*)\)/]($1)/g;
    push @lines, '', split(/\n/, main::rstrip($text));
    $self->write_concept($concept, \@lines, $description);
    return;
}

sub write_indexes {
    my ($self)     = @_;
    my $m          = $self->{m};
    my %subsystems = map { main::subsystem_of($_) => 1 } keys %{$m->{files}};
    my @root = (
                '---', 'okf_version: ' . main::yaml_str($OKF_VERSION), '---', '', '# Netatalk source knowledge bundle',
                '',
                'Generated from Doxygen XML; see [Build record](build.md) for the commit and counts. '
                . 'Subsystem concepts summarize dependencies; file concepts list functions, types and edges.', '',
                '# Read first', '', "* [OKF Knowledge Graph](reading.md) - $self->{descriptions}{'reading.md'}", '',
                '# Build',      '', "* [Build record](build.md) - $self->{descriptions}{'build.md'}",            '',
                '# Subsystems', ''
    );
    for my $name (sort keys %subsystems) {
        push @root,
          "* [$name](" . $self->subsystem_concept($name) . ") - $self->{descriptions}{$self->subsystem_concept($name)}";
    }
    push @root, '', '# Sources', '', '* [src/](src/) - one concept per scanned C source or header', '',
      '# Graph data', '', '* [graph.json](graph.json) - nodes and edges for tooling';
    main::write_file("$self->{out}/index.md", join("\n", @root) . "\n");
    my %dirs;
    for my $concept (keys %{$self->{descriptions}}) {
        next unless $concept =~ m{^src/};
        my $parent = main::dir_of($concept);
        push @{$dirs{$parent}{concepts}}, $concept;
        while ($parent ne '' && $parent ne 'src') {
            my $grand = main::dir_of($parent);
            $dirs{$grand}{dirs}{$parent} = 1;
            $parent = $grand;
        }
    }
    for my $dir (sort keys %dirs) {
        my $entry = $dirs{$dir};
        my @lines = ("# $dir", '');
        for my $sub (sort keys %{$entry->{dirs} // {}}) {
            my $base = main::base_of($sub);
            if (exists $self->{descriptions}{"$sub.md"}) {
                push @lines, "* [$base]($base.md) - $self->{descriptions}{\"$sub.md\"} Files in [$base/]($base/).";
            } else {
                push @lines, "* [$base/]($base/) - directory";
            }
        }
        for my $concept (sort @{$entry->{concepts} // []}) {
            next if exists $entry->{dirs}{substr($concept, 0, -3)};
            my $base = main::base_of($concept);
            push @lines, '* [' . substr($base, 0, -3) . "]($base) - $self->{descriptions}{$concept}";
        }
        main::write_file("$self->{out}/$dir/index.md", join("\n", @lines) . "\n");
    }
    return;
}

sub write_graph {
    my ($self) = @_;
    my $m = $self->{m};
    my (@nodes, @edges);
    for my $path (sort keys %{$m->{files}}) {
        push @nodes, {id => $path, kind => 'file', subsystem => main::subsystem_of($path)};
        for my $inc (sort { $a->[0] cmp $b->[0] } @{$m->{files}{$path}{includes}}) {
            push @edges, {from => $path, to => $inc->[1], kind => 'include'} if defined $inc->[1];
        }
    }
    for my $key (sort main::by_key keys %{$m->{functions}}) {
        my $fn = $m->{functions}{$key};
        my ($name, $home) = main::key_parts($key);
        my $fid = "$home#$name";
        push @nodes, {
                      id         => $fid,
                      kind       => 'function',
                      file       => $home,
                      static     => $fn->{static}                    ? JSON::PP::true       : JSON::PP::false,
                      line       => $fn->{bodystart}                 ? 0 + $fn->{bodystart} : undef,
                      documented => $self->plain($fn->{brief}) ne '' ? JSON::PP::true       : JSON::PP::false,
        };
        for my $callee (sort main::by_key keys %{$fn->{calls}}) {
            my ($cn, $ch) = main::key_parts($callee);
            push @edges, {from => $fid, to => "$ch#$cn", kind => 'call'};
        }
        for my $var (sort main::by_key keys %{$fn->{uses}}) {
            my ($vn, $vh) = main::key_parts($var);
            push @edges, {from => $fid, to => "$vh#$vn", kind => 'use'};
        }
        for my $impl (sort main::by_key keys %{$fn->{dispatch_calls}}) {
            my ($in, $ih) = main::key_parts($impl);
            for my $fkey (sort main::by_key keys %{$fn->{dispatch_calls}{$impl}}) {
                push @edges, {from => $fid, to => "$ih#$in", kind => 'dispatch', via => (main::key_parts($fkey))[0]};
            }
        }
        for my $src (sort main::by_key keys %{$fn->{doc_in}}) {
            my ($sn, $sh) = main::key_parts($src);
            push @edges, {from => "$sh#$sn", to => $fid, kind => 'doc'};
        }
    }
    for my $key (sort main::by_key keys %{$m->{variables}}) {
        my $var = $m->{variables}{$key};
        next unless %{$var->{targets}};
        my ($name, $home) = main::key_parts($key);
        my $vid = "$home#$name";
        push @nodes, {id => $vid, kind => 'table', file => $home};
        for my $target (sort main::by_key keys %{$var->{targets}}) {
            my ($tn, $th) = main::key_parts($target);
            my $edge = {from => $vid, to => "$th#$tn", kind => 'table'};
            $edge->{field} = $m->{fields}{$var->{fields}{$target}}{name} if exists $var->{fields}{$target};
            push @edges, $edge;
        }
    }
    my $json    = JSON::PP->new->canonical->indent->indent_length(0)->space_after->ascii;
    my $encoded = $json->encode({commit => $self->{commit}, nodes => \@nodes, edges => \@edges});
    main::write_file("$self->{out}/graph.json", main::rstrip($encoded) . "\n");
    return scalar @edges;
}

package main;

# Conformance and link check: frontmatter with type on every concept, resolvable links and anchors.
sub check {
    my ($out) = @_;
    my @files;
    find(sub { push @files, $File::Find::name if -f $_ && /\.md$/ }, $out);
    @files = sort @files;
    my (%anchors, @problems, @concepts);
    for my $file (@files) {
        my $rel  = substr($file, length($out) + 1);
        my $text = read_file($file);
        my %heads;
        for my $line (grep {/^#/} split(/\n/, $text)) {
            (my $heading = $line) =~ s/^#+//;
            $heads{anchor(trim($heading))} = 1;
        }
        $anchors{$rel} = \%heads;
        my $base = base_of($rel);
        if ($base eq 'index.md' || $base eq 'log.md') {
            push @problems, "$rel: reserved file with frontmatter" if $text =~ /^---/ && $rel ne 'index.md';
            next;
        }
        push @concepts, $rel;
        my $body = substr($text, 4);
        if ($text !~ /^---\n/ || index($body, "\n---\n") < 0) {
            push @problems, "$rel: missing frontmatter";
            next;
        }
        my $header = substr($body, 0, index($body, "\n---\n"));
        push @problems, "$rel: missing type" unless $header =~ /^type: \S/m;
        push @problems, "$rel: unstamped" if index($text, '__GENERATED_AT__') >= 0;
    }
    for my $file (@files) {
        my $rel  = substr($file, length($out) + 1);
        my $text = read_file($file);
        while ($text =~ /\]\(([^)\s]+)\)/g) {
            my $target = $1;
            next if $target =~ m{^(https?://|#)};
            my ($target_path, $fragment) = split(/#/, $target, 2);
            $fragment //= '';
            my $resolved = $target_path ne '' ? norm_path(join('/', dir_of($rel), $target_path)) : $rel;
            $resolved = "$resolved/index.md" if $target_path =~ m{/$};
            if (!-e "$out/$resolved") {
                push @problems, "$rel: broken link $target";
            } elsif ($fragment ne '' && !exists $anchors{$resolved}{$fragment}) {
                push @problems, "$rel: missing anchor $target";
            }
        }
    }
    print {*STDERR} "check: $_\n" for @problems[0 .. ($#problems < 49 ? $#problems : 49)];
    printf "check: %d concepts, %d problems\n", scalar @concepts, scalar @problems;
    return !@problems;
}

sub main {
    my %opt = (source => '.', 'repo-url' => 'https://github.com/Netatalk/netatalk', branch => 'main');
    GetOptions(\%opt, 'source=s', 'out=s', 'previous=s', 'commit=s', 'commit-date=s', 'repo-url=s', 'branch=s', 'xml=s')
      or die
"usage: okf_from_doxygen.pl --out DIR --commit SHA --commit-date ISO8601 [--source DIR] [--previous DIR] [--xml DIR]\n";
    for my $required (qw(out commit commit-date)) {
        die "--$required is required\n" unless defined $opt{$required};
    }
    my $source = abs_path($opt{source});

    # The output directory is replaced wholesale, so it must not be or contain the source tree
    # or the previous bundle the timestamps are carried over from.
    my $out = abs_path($opt{out}) // $opt{out};
    die "--out must not be the filesystem root\n" if $out eq '/';
    for my $guarded (grep {defined} $source, defined $opt{previous} ? abs_path($opt{previous}) : undef) {
        die "--out must not be or contain $guarded\n" if $guarded eq $out || index("$guarded/", "$out/") == 0;
    }
    my ($xml_dir, @predefined, $doxygen_version, $tmp);
    if (defined $opt{xml}) {
        $xml_dir         = $opt{xml};
        $doxygen_version = 'existing XML';
    } else {
        $tmp             = tempdir('okf-doxygen-XXXXXX', TMPDIR => 1, CLEANUP => 1);
        $xml_dir         = "$tmp/xml";
        @predefined      = run_doxygen($source, $xml_dir, substr($opt{commit}, 0, 12));
        $doxygen_version = (split(' ', qx(doxygen --version)))[0];
    }
    my $model = load($xml_dir, $source);
    remove_tree($opt{out}) if -e $opt{out};
    make_path($opt{out});
    my $previous = defined $opt{previous} && -d $opt{previous} ? $opt{previous} : undef;
    my $writer = Writer->new(
                             model       => $model,
                             out         => $opt{out},
                             repo_url    => $opt{'repo-url'},
                             branch      => $opt{branch},
                             commit      => $opt{commit},
                             commit_date => $opt{'commit-date'},
                             previous    => $previous,
    );
    $writer->write_file($_) for sort keys %{$model->{files}};
    my %by_subsystem;
    push @{$by_subsystem{subsystem_of($_)}}, $_ for keys %{$model->{files}};
    $writer->write_subsystem($_, $by_subsystem{$_}) for sort keys %by_subsystem;
    my $edge_count = $writer->write_graph();
    my $tables     = grep { %{$_->{targets}} } values %{$model->{variables}};
    my @functions  = values %{$model->{functions}};
    my @stats = (
                 ['Files',                scalar keys %{$model->{files}}],
                 ['Subsystems',           scalar keys %by_subsystem],
                 ['Functions',            scalar @functions],
                 ['Documented functions', scalar grep { $writer->brief($_->{brief}) ne '' } @functions],
                 ['Types',                scalar keys %{$model->{structs}}],
                 ['Function tables',      $tables],
                 ['Call edges',           sum(map { scalar keys %{$_->{calls}} } @functions)],
                 ['Variable-use edges',   sum(map { scalar keys %{$_->{uses}} } @functions)],
                 ['Table edges',          sum(map { scalar keys %{$_->{targets}} } values %{$model->{variables}})],
                 [
                  'Dispatch edges',
                  sum(
                      map {
                          sum(map { scalar keys %$_ } values %{$_->{dispatch_calls}})
                      } @functions
                  )
                 ],
                 ['Fields called through',      scalar grep { %{$_->{callers}} } values %{$model->{fields}}],
                 ['Documented-reference edges', sum(map { scalar keys %{$_->{doc_out}} } @functions)],
                 ['Graph edges',                $edge_count],
    );
    $writer->write_build($doxygen_version, \@predefined, \@stats);
    $writer->write_reading("$source/$READING_GUIDE");
    $writer->write_indexes();
    print "$_->[0]: $_->[1]\n" for @stats;
    exit 1 unless check($opt{out});
    return;
}

sub sum {
    my $total = 0;
    $total += $_ for @_;
    return $total;
}

main();
