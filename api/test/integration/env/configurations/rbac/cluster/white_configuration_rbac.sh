#!/usr/bin/env bash

sed -i 's,"mode": \("white"\|"black"\),"mode": "white",g' /var/ossec/framework/python/lib/python3.7/site-packages/api-3.11.0-py3.7.egg/api/configuration.py
sed -i "s:    # policies = RBAChecker.run_testing():    policies = RBAChecker.run_testing():g" /var/ossec/framework/python/lib/python3.7/site-packages/wazuh-3.12.0-py3.7.egg/wazuh/rbac/preprocessor.py
permissions='[{"actions":["cluster:read_config"],"resources":["node:id:master-node","node:id:worker2"],"effect":"allow"},{"actions":["cluster:restart"],"resources":["node:id:worker1"],"effect":"allow"},{"actions":["cluster:status"],"resources":["*"],"effect":"allow"},{"actions":["cluster:read_file"],"resources":["file:path:etc/ossec.conf","file:path:etc/rules/local_rules.xml","file:path:ruleset/rules/0350-amazon_rules.xml","node:id:master-node","node:id:worker2"],"effect":"allow"},{"actions":["cluster:upload_file"],"resources":["file:path:etc/decoders/local_decoder.xml","file:path:etc/rules/ruleset/rules/0350-amazon_rules.xml","file:path:etc/decoders/test_decoder.xml","node:id:master-node","node:id:worker2"],"effect":"allow"},{"actions":["cluster:delete_file"],"resources":["file:path:ruleset/decoders/0005-wazuh_decoders.xml","node:id:master-node","node:id:worker2"],"effect":"allow"}]'
awk -v var="${permissions}" '{sub(/testing_policies = \[\]/, "testing_policies = " var)}1' /var/ossec/framework/python/lib/python3.7/site-packages/wazuh-3.12.0-py3.7.egg/wazuh/rbac/auth_context.py >> /var/ossec/framework/python/lib/python3.7/site-packages/wazuh-3.12.0-py3.7.egg/wazuh/rbac/auth_context1.py
cat /var/ossec/framework/python/lib/python3.7/site-packages/wazuh-3.12.0-py3.7.egg/wazuh/rbac/auth_context1.py > /var/ossec/framework/python/lib/python3.7/site-packages/wazuh-3.12.0-py3.7.egg/wazuh/rbac/auth_context.py