#!/usr/bin/bash
HOST="$1"
#echo $HOST
RAWSSL=$(openssl s_client -connect $HOST:443 2>/dev/null </dev/null)
#echo $RAWSSL
CERTONLY=$(echo $RAWSSL | sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p')
echo $CERTONLY
#RAWX509="$(echo $CERTONLY | openssl x509 -noout -fingerprint -sha1)"
#echo $RAWX509
#openssl s_client -connect $HOST 2>/dev/null </dev/null | sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p' | openssl x509 -noout -fingerprint -sha1
#echo $(openssl s_client -connect "${HOST}" 2>/dev/null </dev/null | sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p' | openssl x509 -noout -fingerprint -sha1)
