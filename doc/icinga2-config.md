[TOC]

# Icinga 2 Configuration Syntax

# 1.1 Object Definition

Icinga 2 features an object-based configuration format. In order to define objects the „object“ keyword is used:

	object Host "host1.example.org" {
		alias = "host1",

		check_interval = 30,
		retry_interval = 15,

		macros = {
			address = "192.168.0.1"
		}
	}

**Note**: The Icinga 2 configuration format is agnostic to whitespaces and new-lines.

Each object is uniquely identified by its type („Host“) and name („localhost“). Objects can contain a  comma-separated list of property declarations.

## 1.2 Data Types

### 1.2.1 Number

A floating-point number.

Examples:

	27
	-27.3

### 1.2.2 String

Example:

A string. No escape characters are supported at present – though this will likely change.

	"Hello World!"

### 1.2.3 Expression List

A list of expressions that when executed has a dictionary as a result.

Example:

	{
		address = "192.168.0.1",
		port = 443
	}

## 1.3 Operators

### 1.3.1 The = Operator

Sets an attribute to the specified value.

Example:

	{
		a = 5,
		a = 7
	}

In this example "a" is 7 after executing this expression list.

### 1.3.2 The += Operator

Inserts additional items into a dictionary. Keys are overwritten if they already exist.

Example:

	{
		a = { "hello" },
		a += { "world" }
	}

In this example "a" is a dictionary that contains both "hello" and "world" as elements.

The += operator is only applicable to dictionaries. Support for numbers might be added later on.

### 1.3.3 The -= Operator

Removes items from a dictionary.

Example:

	{
		a = { "hello", "world" },
		a -= { "world" }
	}

In this example "a" is a dictionary that only contains "hello" as an element.

**Note**: The -= operator is not currently implemented.

### 1.3.4 The *= Operator

Multiplies an attribute with a number.

Example:

	{
		a = 60,
		a *= 5
	}

In this example "a" contains the value 300.

**Note**: The *= operator is not currently implemented.


### 1.3.5 The /= Operator

Divides an attribute with a number.

Example:

	{
		a = 300,
		a /= 5
	}

In this example "a" contains the value 60.

**Note**: The /= operator is not currently implemented.

## 1.4 Property Shortcuts

### 1.4.1 Value Shortcut

Example:

	{
		"hello"
	}

This is equivalent to writing:

	{
		hello = "hello"
	}

### 1.4.2 Index Shortcut

Example:

	{
		hello["key"] = "world"
	}

This is equivalent to writing:

	{
		hello += {
			key = "world"
		}
	}

## 1.5 Specifiers

### 1.5.1 The abstract specifier

Identifies the object as a template which can be used by other object definitions. The object will not be instantiated on its own.

Example:

	abstract object Host "test" {

	}

### 1.5.2 The local specifier

Disables replication for this object. The object will not be sent to remote Icinga instances.

Example:

	local object Host "test" {

	}

## 1.6 Inheritance

Objects can inherit properties from one or more other objects:

	abstract object Host "default-host" {
		check_interval = 30,

		macros = {
			color = "red"
		}
	}

	abstract object Host "test-host" {
		macros += {
			color = "blue"
		}
	}
		

	object Host "localhost" inherits "test-host" {
		macros += {
			address = "127.0.0.1",
			address6 = "::1"
		}
	}

**Note**: The „default-host“ and „test-host“ objects is marked as templates using the „abstract“ keyword. Parent objects do not necessarily have to be „abstract“ though in general they are.

**Note**: The += operator is used to insert additional properties into the macros dictionary. The final dictionary contains all 3 macros and the property „color“ has the value „blue“.

Parent objects are resolved in the order they're specified using the „inherits“ keyword. Parent objects must already be defined by the time they're used in an object definition.

## 1.7 Comments

The Icinga 2 configuration format supports C/C++-style comments

Example:

	/*
	 This is a comment.
	 */
	object Host "localhost" {
		check_interval = 30, // this is also a comment.
		retry_interval = 15
	}

## 1.8 Includes

Other configuration files can be included using the „#include“ directive. Paths must be relative to the configuration file that contains the „#include“ keyword:

Example:

	#include "some/other/file.conf"
