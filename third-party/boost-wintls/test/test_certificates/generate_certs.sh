# This script is used to generate all the files in ./gen using the config files from ./conf.
# These files are used from the unit tests in certificate_test.cpp

# Clear all previously generated files
rm -rf gen
mkdir -p gen

# Generate self signed Root certificate valid for 100 years
openssl req -days 36525 -nodes -new -x509 -keyout gen/ca_root.key -out gen/ca_root.crt -config conf/ca_root.conf
# Request intermediate certificate
openssl req -nodes -new -keyout gen/ca_intermediate.key             -out gen/ca_intermediate.csr             -config conf/ca_intermediate.conf
# Request leaf certificates
openssl req -nodes -new -keyout gen/leaf.key                        -out gen/leaf.csr                        -config conf/leaf.conf
openssl req -nodes -new -keyout gen/leaf_ocsp.key                   -out gen/leaf_ocsp.csr                   -config conf/leaf_ocsp.conf
openssl req -nodes -new -keyout gen/leaf_ocsp_revoked.key           -out gen/leaf_ocsp_revoked.csr           -config conf/leaf_ocsp.conf
openssl req -nodes -new -keyout gen/ocsp_signer_ca_intermediate.key -out gen/ocsp_signer_ca_intermediate.csr -config conf/ocsp_signer_ca_intermediate.conf

# Setup for signing the subordinate certificates
touch gen/certindex
echo 01 > gen/certserial
echo 01 > gen/crlnumber
mkdir -p gen/newcerts

# Sign intermediate certificate
openssl x509 -req -CA gen/ca_root.crt -CAkey gen/ca_root.key -CAcreateserial  -days 36525 -extensions req_v3_ca -extfile conf/ca_intermediate.conf -in gen/ca_intermediate.csr -out gen/ca_intermediate.crt 
# Sign leaf certificates
SIGN_LEAF_COMMON_PARAMS="ca  -config conf/ca_intermediate.conf -extensions req_v3_usr -batch"
openssl $SIGN_LEAF_COMMON_PARAMS -extfile conf/leaf.conf                        -in gen/leaf.csr                        -out gen/leaf.crt
openssl $SIGN_LEAF_COMMON_PARAMS -extfile conf/leaf_ocsp.conf                   -in gen/leaf_ocsp.csr                   -out gen/leaf_ocsp.crt
openssl $SIGN_LEAF_COMMON_PARAMS -extfile conf/leaf_ocsp.conf                   -in gen/leaf_ocsp_revoked.csr           -out gen/leaf_ocsp_revoked.crt
openssl $SIGN_LEAF_COMMON_PARAMS -extfile conf/ocsp_signer_ca_intermediate.conf -in gen/ocsp_signer_ca_intermediate.csr -out gen/ocsp_signer_ca_intermediate.crt

# Create certificate chains for the leaf certificates
cat gen/leaf.crt              gen/ca_intermediate.crt gen/ca_root.crt > gen/leaf_chain.pem
cat gen/leaf_ocsp.crt         gen/ca_intermediate.crt gen/ca_root.crt > gen/leaf_ocsp_chain.pem
cat gen/leaf_ocsp_revoked.crt gen/ca_intermediate.crt gen/ca_root.crt > gen/leaf_ocsp_revoked_chain.pem

# Generate empty CRL
openssl ca -config conf/ca_root.conf         -gencrl -out gen/ca_root_empty.crl.pem
openssl ca -config conf/ca_intermediate.conf -gencrl -out gen/ca_intermediate_empty.crl.pem

# Revoke leaf.crt and leaf_ocsp_revoked.crt
openssl ca -config conf/ca_intermediate.conf -revoke gen/leaf.crt
openssl ca -config conf/ca_intermediate.conf -revoke gen/leaf_ocsp_revoked.crt

# Generate CRL including the revoked certificates
openssl ca -config conf/ca_intermediate.conf -gencrl -out gen/ca_intermediate_leaf_revoked.crl.pem
