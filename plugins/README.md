## Icinga 2 plugins for Windows

This collection of plugins is intended to provide basic functionality checks on windows machines.  
They (mostly) conform to the [nagios developer guidelines](https://nagios-plugins.org/doc/guidelines.html), 
returning adequate exit codes and printing a pertinent string with performance data.


### Intallation

The plugins are installed as part of Icinga 2.


### Requirements

- Boost 1.41.0
- Windows Vista, Windows Server 2008 or newer


### Usage

Call a plugin with the "--help" option to receive information about its usage.  
Most of them don't need any parameters to but all of them have a -w (warning) and -c (critical) option. 
Those accept, if not otherwise specified, value or percentage based thresholds or threshold ranges.  

A few examples:  
*./check_command.exe -w 12 -c !60%*  
Adds a warning threshold of 12 and an inversed critical threshold of 60%  

*./check_command.exe -w ![20%-80%] -c [0%-40%]*  
The warning threshold is outside of 20% to 80% and the critical threshold is the range from 0% to 40%.  
A critical state always overwrites a warning state, meaning the check would be critical with a value of 30%.


### License

Icinga 2  
Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)

This program is free software; you can redistribute it and/or  
modify it under the tems of the GNU General Public License  
as published by the Free Software Foundation; either version 2  
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,  
but WITHOUT ANY WARRANTY; without even the implied warranty of  
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License  
 along with this program; if not, write to the Free Software Foundation  
 Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
