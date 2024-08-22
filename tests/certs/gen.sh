#!/bin/sh

# Copyright Jerily LTD. All Rights Reserved.
# SPDX-FileCopyrightText: 2024 Neofytos Dimitriou (neo@jerily.cy)
# SPDX-License-Identifier: MIT.

rm -f ca-*.pem server-*.pem client-*.pem

set -x
openssl genrsa -out ca-key.pem 4096
openssl req -new -text -x509 -nodes -days 365000 -key ca-key.pem -out ca-cert.pem \
    -subj "/C=CA/ST=Wonderland/L=Alice Home/OU=Test CA/O=valkey-tcl"

openssl req -text -newkey rsa:4096 -nodes -keyout server-key.pem -out server-req.pem \
    -subj "/C=CA/ST=Wonderland/L=Alice Home/OU=Test Server/O=valkey-tcl/CN=127.0.0.1"

echo 'subjectAltName=DNS:localhost,IP:127.0.0.1' > server.ext
openssl x509 -req -text -days 365000 -set_serial 01 -in server-req.pem -out server-cert.pem \
    -CA ca-cert.pem -CAkey ca-key.pem \
    -extfile server.ext
rm -f server.ext

openssl req -text -newkey rsa:4096 -nodes -keyout client-key.pem -out client-req.pem \
    -subj "/C=CA/ST=Wonderland/L=Alice Home/OU=Test Client/O=valkey-tcl/CN=127.0.0.1"
openssl x509 -req -text -days 365000 -set_serial 01 -in client-req.pem -out client-cert.pem -CA ca-cert.pem -CAkey ca-key.pem
