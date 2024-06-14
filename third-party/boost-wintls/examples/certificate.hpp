//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <string>

#ifndef BOOST_WINTLS_EXAMPLES_CERTIFICATE_HPP
#define BOOST_WINTLS_EXAMPLES_CERTIFICATE_HPP

const std::string x509_certificate =
  "-----BEGIN CERTIFICATE-----\n"
  "MIIDdzCCAl+gAwIBAgIUSZTPaL6edK/wEZNYf/9Scqlxx+0wDQYJKoZIhvcNAQEL\n"
  "BQAwSzELMAkGA1UEBhMCREsxEzARBgNVBAcMCkNvcGVuaGFnZW4xEzARBgNVBAoM\n"
  "ClJlcHRpbGljdXMxEjAQBgNVBAMMCWxvY2FsaG9zdDAeFw0xOTA2MDEyMDMyMzRa\n"
  "Fw0zOTA2MDEyMDMyMzRaMEsxCzAJBgNVBAYTAkRLMRMwEQYDVQQHDApDb3Blbmhh\n"
  "Z2VuMRMwEQYDVQQKDApSZXB0aWxpY3VzMRIwEAYDVQQDDAlsb2NhbGhvc3QwggEi\n"
  "MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDasWBBpRZTezyOv+8l2db1BOUH\n"
  "D2T4s6MlFoq+Vh6K37XgCGmDjXQP/GaDBxdCuC4HoL0ABXR9/upYPdQzSkniPelC\n"
  "3si8S7USA3uEYAvhnprVoAWqZebN0ljgic7kiCJsBLSBjshzVFGga7MygBLA1ozx\n"
  "sVLSz4FPH/lcKUmjsZPkhBeEY9ENuyk7JX3bLtAHQy2mLhkTse5LFv+ZE+RyaX2l\n"
  "PliDKrScanfXItpbVlJPFVYkRs1l/dA3KJsETzLvYrAseguT0GKU7ticVW+mvhjL\n"
  "Q8rNt95xMgo/hy3EESb2yU5hYo9hFidqMoTnbgLvp3dRxBy8kgLTAtCK/+gNAgMB\n"
  "AAGjUzBRMB0GA1UdDgQWBBQxSLSxzTHyTEQthNoyMPeCMArBCzAfBgNVHSMEGDAW\n"
  "gBQxSLSxzTHyTEQthNoyMPeCMArBCzAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3\n"
  "DQEBCwUAA4IBAQBME6UTgiWj6FfnjCaAOigkytrDmfEhkZiaXzzSoC0NJt+hyf5s\n"
  "Jqimj4/JJlhaFRKd6A+J1T04Sg4JwRMXqmmsXZN7NRZ250OrtPVt3m1+vvNqJOYW\n"
  "b24/E2dvhS0feqSbKIar7XnBFcLDw/w4bGmpztoywYmHtZ0USzNIOCuGAogdH/wL\n"
  "55GvqpClYgBGDGT39fi75GeFNJqa7KUqvTm7FJeUq6oYkkKs+Z08u1GSKb3kekBb\n"
  "nJkQwUG/xoU1XFNaFdeKuTpjVllVITRAFgjYF4Oy5dZJix8QgHJuG3VbD1VYGoeU\n"
  "chJ23AD7h+jmt7DOOTGspsFUqxXmMRiM+Q3Y\n"
  "-----END CERTIFICATE-----\n";

const std::string rsa_key =
  "-----BEGIN PRIVATE KEY-----\n"
  "MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDasWBBpRZTezyO\n"
  "v+8l2db1BOUHD2T4s6MlFoq+Vh6K37XgCGmDjXQP/GaDBxdCuC4HoL0ABXR9/upY\n"
  "PdQzSkniPelC3si8S7USA3uEYAvhnprVoAWqZebN0ljgic7kiCJsBLSBjshzVFGg\n"
  "a7MygBLA1ozxsVLSz4FPH/lcKUmjsZPkhBeEY9ENuyk7JX3bLtAHQy2mLhkTse5L\n"
  "Fv+ZE+RyaX2lPliDKrScanfXItpbVlJPFVYkRs1l/dA3KJsETzLvYrAseguT0GKU\n"
  "7ticVW+mvhjLQ8rNt95xMgo/hy3EESb2yU5hYo9hFidqMoTnbgLvp3dRxBy8kgLT\n"
  "AtCK/+gNAgMBAAECggEASdz07NcMZl/OQUyUQk2EK7def3b0nIdXx/QIImdF45PR\n"
  "gvx0XslM9QVDvmeLtK4uZcclbrdo9BFAJ1OiszwZHj/Y5AwI8ogDfTUN59Tkzmxa\n"
  "UWK95yKJxOSRviztYwST07X3HXcTPtiwxST7HkhhjR4p9ov0tFz/iLD88OLFC3MT\n"
  "8IyCb86m8nShrIfsbgqQrUPHPob78MsbqrzWqXOUv0ryoa2Bj3T3jsU6Sz2Kgqyo\n"
  "I0V4fC9fXk7WFBXLiIt9H5CQPmzbMYG3+BjQpqP41NR2uMFT+N+Sgtj61rNl0FA3\n"
  "4rHIIKBbsYWcKSg+l6qCTt5xyOiRDRt/vWdGSfNJRQKBgQDutm3wY7eNDFaMMqRb\n"
  "nTgtKxjqW4RKbGvub0zJHlP4iCQvZV/v4PAKtnBPPB4UcgIeKAoXqBFQfErZt0E6\n"
  "7W6/w3OrmslHYSzwO9HwWpHOCUiwI+sLivpupHgRopRELX39bAmIkd0BcifIzyuC\n"
  "Btpd/6aYGc9WYS3NeTm24so2IwKBgQDqh8tKS1ki7VrloJgRgeAfIdRrKmSp9YBO\n"
  "jSC3o3IMqIB5n8GBCYCEuu7qky3hH1Byd7qIMge3n12s2iNzTp8nz5Zt3E8rom46\n"
  "kgFuSHEYXu7FCRjINcHflgFAlpXOGb/Kc8uv5quFGUe0Ts6HXCDwYsITa7IW2CcN\n"
  "Z6zoSCQUDwKBgQDPbJ/dOZi+LFFsI3kjzlqJDSDqS7mJPesPVZFAVWUb2ZivwoP5\n"
  "mdibRmoSv6dXlrV2ZM5YPgdFi6sywYUh4jzQztJM9AQgTTVSTnifROPbR7/spllQ\n"
  "P2RbDfjzQfVZxLnsopsqG60R25Lsb/BbXP8UnNey5QKACZNWLxdSboDNRwKBgEI3\n"
  "5SYs8BX0fl2nYkhPK0CfBKLdbV6venKzVjGDbIg2a0/r027jh+3x+dMxixqtBMHN\n"
  "HFWoCpXF4WUUkj3UTQuoiQir0462ZfkTkGPbAFOpOjFXyC9/PiYq7F+YJOP0UTqQ\n"
  "R0p7DqKd6Kj4N0fwszwsJi/lkHryvNQEGcXb2JPxAoGAJupWq9v9/LkZ8wTTeze+\n"
  "01uqfC6T/qYPBxLjleuzMmfEoKhqzLyFzW+r4K5TnYz7jxP52KUSHR3r2lXdMasQ\n"
  "kaTAid5ZjQUt5MkjI3W0N3tJhdzTH+rdkOWb+59ANECfhr1D0/o3GjSx2RW1EhtG\n"
  "hSWgRxNM62fdbmxZ+JrxHCE=\n"
  "-----END PRIVATE KEY-----\n";

#endif // BOOST_WINTLS_EXAMPLES_CERTIFICATE_HPP
