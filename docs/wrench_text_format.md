# Wrench Text Format

This is the structured text format that is used by Wrench's asset system to store metadata (the .asset files). It's also used in some other places such as the gameinfo.txt file. These files are comprised of a tree of nodes, where the root node is implicitly defined. Each node is comprised of a type name, a tag name, a set of attributes, and a set of child nodes.

## Example

An example file is shown below:

	Type the_tag {
		number_attribute: 1.23
		boolean_attribute: true
		
		the_child {
			string_attribute: "This is a string."
		}
		
		Example nested.inner {
			array_attribute: ["This" "is" "an" "array."]
		}
	}

Here, we define 5 nodes: the root, the_tag, the_child, nested and inner, where the_tag is a child of the root, the_child is a child of the_tag, nested is a child of the_tag, and inner is a child of nested. In the case of the_tag, the type name is explicitly specified, for the_child it is omitted. The type name of inner is Example and the type name of nested is null.

We additionally define 4 attributes: number_attribute, boolean_attribute, string_attribute and array_attribute. Number attributes can represent floating point numbers or integers, boolean attributes can be either 'true' or 'false', and array attributes can nest arbitrary other kinds of values including other arrays (but not other nodes).

## Identifiers

Identifiers, including type names, tags and attribute names can include upper and lower case English letters, numerical digits and underscores in any order. Additionally, dots can be included but two dots must not appear consecutively, and dots may not appear at the beginning or end of an identifier.

## String Escaping

The following escape codes are supported for strings:

	\t -> tab
	\n -> line feed
	\" -> "
	\\ -> \

Additionally, \\x followed by two hex characters can be used to represent an arbitrary byte. For example, \\x00 represents the null character, \\x41 represents the ASCII A character.

## Whitespace

Outside of strings, whitespace should be ignored, except where it is required to seperate tokens (for example, between type names and tags).

## Grammar

	<body> := <attrib_or_child> <body> | epsilon
	<attrib_or_child> := <attrib> | <child>
	<attrib> := identifier : <value>
	<value> := number | boolean | string | <array>
	<array> := [ <elements> ]
	<elements> := <value> <elements> | epsilon
	<child> := identifier identifier { <body> } | identifier { <body> }

where \<body\> is the starting nonterminal and epsilon represents nothing.
