#!/bin/sh

: ==== start ====

TESTNAME=x509-fail-08
source /testing/pluto/bin/eastlocal.sh

rm /tmp/$TESTNAME/ipsec.d/certs/west*
rm /tmp/$TESTNAME/ipsec.d/crls/ca.crl

iptables -A INPUT -i eth1 -s 192.0.3.0/24 -d 0.0.0.0/0 -j DROP

arp -s 192.0.2.1 10:00:00:dc:bc:01

ipsec setup start
/testing/pluto/bin/wait-until-pluto-started
ipsec auto --add north-east-x509-fail-08

echo done


