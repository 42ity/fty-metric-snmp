# fty-metric-snmp

This 42ITy agent creates metrics and publish them on stream. Rules for creating
metrics are specified with json and lua. All rule files are loaded from one
directory specified by command line parameter. File has to have .rule extension.

Lua interpreter is extended of SNMP functions snmpget and snmpgetnext.

```json
{
    "name" : "linuxload",
    "description" : "Linux 1 min, 5 min and 15 min load.",
    "groups" : ["linuxserver"],
    "evaluation" : "
    function main (host)
        load1m = snmp_get (host, '.1.3.6.1.4.1.2021.10.1.3.1');
        load5m = snmp_get (host, '.1.3.6.1.4.1.2021.10.1.3.2');
        load15m = snmp_get (host, '.1.3.6.1.4.1.2021.10.1.3.3');
        return {
            'load', load1m, '%', 'one minute average load',
            'load5m', load5m, '%', 'five minutes average load',
            'load15m', load15m, '%', 'fifteen minute average load'
        }
    end"
}
```
Note: Yes this is not valid json, but used parser tolerates multiline string.

## Rules
Rule file MUST be in UTF-8 encoding (ASCII is OK of course)
Rules json has following parts
* name - mandatory - name of the rule, SHOULD be ASCII identifier of the rule and
  MUST be unique
* description - optional - user friendly description of the rule
* groups - optional - list of asset groups (extended attribute group.x). Rule will
  be used for all assets that belongs to at least one of listed groups.
* assets - optional - rule will be applied to assets explicitly listed here
* models - optional - rule will be applies to assets of listed model or part number
  (see extended attribute model and device.part)
* evaluation - mandatory - lua code for producing metrics.

You can combine assets, groups and models in one rule.

Lua code MUST have function called main with one parameter. Extended attribute ip.1
is passed to the function when it is evaluated.

Lua code MUST return a table. Table items are
* name of the metric
* value
* units
* description (can be just '')

This list can be repeated, so you can produce more metrics at once. See the example above.

## SNMP

SNMP version and credentials are readed from 42ITy configuration file. They are stored
in lua global variables SNMP_VERSION and SNMP_COMMUNITY_NAME.

lua is extended of two SNMP functions
### snmp_get (oid)
Function returns the value or nil on error. (wrong oid, networking issue, wrong
credentials, timeout, ...)

```lua
-- get system description
systemdescription = snmp_get (host, '.1.3.6.1.2.1.1.0');
```
### snmp_getnext (oid)
Function returns oid and value on success and nil, nil on error
```lua
-- find first 5 items in MIB
oid = '.1'
for i=1,5 do
  oid, value = snmp_getnext(oid)
  print (oid, value)
end
```

## nagios plugins
It is possible to re-use nagios plugins. The concept is simple. Run the plugin, read
the output and exit code. Then produce metric named "nagios.something" with value of
the exitcode and with description containing the plugin output.

Other agent - fty-alert-flexible - checks for metrics called nagios.* and produces
alerts from them.

See nagios-plugin-style.rule for example. Use io.popen to run plugin.
```lua
local plugin = io.popen ('myplugin -H ' .. host);
local output = plugin:read ('*a');
local result = {plugin:close ()}
-- now result[3] contains the exitcode
return {'nagios.myplugin', result[3], '', output}
```
### Performance data
Some nagios plugins produces performance data. You can create additional metrics from
the plugin output.

