# Wrench Text Format

This is the structured text format that is used by Wrench's asset system to store metadata (the .asset files). It's also used in some other places such as the gameinfo.txt file. These files are comprised of a tree of nodes, where the root node is implicitly defined. Each node is comprised of a type name, a tag name, a set of attributes, and a set of child nodes.

## Example

An example file is shown below:

	Type the_tag {
		number_attribute: 1.23
		boolean_attribute: true
		string_attribute: 'This is a string.'
		
		the_child {
			array_attribute: ['This' 'is' 'an ' 'array.']
		}
	}

Here, we define 3 nodes: the root, the_tag and the_child, where the_tag is a child of the root, and the_child is a child of the_tag. In the case of the_tag, the type name is explicitly specified, for the_child it is omitted. We additionally define 4 attributes: number_attribute, boolean_attribute, string_attribute and array_attribute. Number attributes can represent floating point numbers or integers, boolean attributes can be either 'true' or 'false', and array attributes can nest arbitrary other kinds of values (except nodes).

## String Escaping

The following escape codes are supported for strings:

	\t -> tab
	\n -> line feed
	\' -> '
	\\ -> \

Additionally, \\x followed by two hex characters can be used to represent an arbitrary byte. For example, \\x00 represents the null character, \\x41 represents the ASCII A character.
