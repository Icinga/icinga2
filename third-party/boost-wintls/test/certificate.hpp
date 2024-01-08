//
// Copyright (c) 2022 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef WINTLS_CERTIFICATE_HPP
#define WINTLS_CERTIFICATE_HPP

#include <string>

const std::string test_certificate =
  "-----BEGIN CERTIFICATE-----\n"
  "MIIDbzCCAlegAwIBAgIUaIFyQGvp8/q1J7Lun753bvRBjKQwDQYJKoZIhvcNAQEL\n"
  "BQAwRzELMAkGA1UEBhMCREsxEzARBgNVBAcMCkNvcGVuaGFnZW4xDzANBgNVBAoM\n"
  "BldpblRMUzESMBAGA1UEAwwJbG9jYWxob3N0MB4XDTIyMDYyNTIwMzg0OVoXDTQ5\n"
  "MTExMDIwMzg0OVowRzELMAkGA1UEBhMCREsxEzARBgNVBAcMCkNvcGVuaGFnZW4x\n"
  "DzANBgNVBAoMBldpblRMUzESMBAGA1UEAwwJbG9jYWxob3N0MIIBIjANBgkqhkiG\n"
  "9w0BAQEFAAOCAQ8AMIIBCgKCAQEAy6b79MKE2mdt1h7GtfFmol68HI/Q3tOsPHaJ\n"
  "cDv7PfflJofqlae1SPFAs4OspQ7IAjcwuictiyE9MbPo5vgiAEctZ2aP2PbF99AP\n"
  "XAtkeLLLF5zxqVTbQtmEt147aycALe7WddVNQqlAFQ7gd6VCo/1jZMRlwNHPT1Oi\n"
  "PkowAfVpDYCIwuL/0Rc6bRkh8p6J5Oi6S4sWcT7e6oo81NOKwyK6h2WJ8c01tFWp\n"
  "G26XySopaH3u2HT+3Elg0lIdaaAm4EhZMysYIsPli+o2OiM4Dm2qGwK/VxeZz83W\n"
  "aFS8jYPZQkB2fWOAk3Lm/hLLkSj5Z0qcin4543DOREO3b9J4ZQIDAQABo1MwUTAd\n"
  "BgNVHQ4EFgQU8TcsJp8OfaZJvBIfCOz5fqnhztwwHwYDVR0jBBgwFoAU8TcsJp8O\n"
  "faZJvBIfCOz5fqnhztwwDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOC\n"
  "AQEAsUryE+jjeg/8xhwvthH+GJVjQSVvAbJjd1PQMLo3Yy578a1UN6aBOELwRe4W\n"
  "FgR3ASRKhrc/mnSju0eLSF69lSWStjJC+ZNlZRuwOS/Ik9A6BAe6sOARpKPV1rV2\n"
  "uWs8PxsWqPuTdICri91bEpmq1jntNTRgXmFW/hQR2ndy03h9hUHd0Xn70mlESayJ\n"
  "2s6qnAMp+OzlTtWO6eSYYajYQdmsmIpby5eGjPzPNhoIVPKtSUa6eN5GltlW74qu\n"
  "75w4XWlNO3J0AX4ZoWc7fztsWkgTX41Vv48lkxPLqLxQQTQsN6wdZ/c/sdSAXK0x\n"
  "GinlCF73ogVSHXdtNMGrQP1n8w==\n"
  "-----END CERTIFICATE-----\n";

const std::string test_key =
  "-----BEGIN PRIVATE KEY-----\n"
  "MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDLpvv0woTaZ23W\n"
  "Hsa18WaiXrwcj9De06w8dolwO/s99+Umh+qVp7VI8UCzg6ylDsgCNzC6Jy2LIT0x\n"
  "s+jm+CIARy1nZo/Y9sX30A9cC2R4sssXnPGpVNtC2YS3XjtrJwAt7tZ11U1CqUAV\n"
  "DuB3pUKj/WNkxGXA0c9PU6I+SjAB9WkNgIjC4v/RFzptGSHynonk6LpLixZxPt7q\n"
  "ijzU04rDIrqHZYnxzTW0VakbbpfJKilofe7YdP7cSWDSUh1poCbgSFkzKxgiw+WL\n"
  "6jY6IzgObaobAr9XF5nPzdZoVLyNg9lCQHZ9Y4CTcub+EsuRKPlnSpyKfjnjcM5E\n"
  "Q7dv0nhlAgMBAAECggEBAKRhZa/7rsang5W4g8ZqUuCuvQIE56BklPq851TrZXFw\n"
  "fctrG+OuWfrFmOcNWrZkRvba2372jqFltAJBaLW+BZvZ2AFFXMjQ75yGmU8/dtqh\n"
  "3qJxsPJwJwc/kgt8iVOFSHTK+tpj0JgFC0+0EWUhxLefmLHGgSdxcvdh12yV70g0\n"
  "APtKnCax/QznmW1XbXiEqP9n5mspOeC2LcJLKdH6PRiCeN+AnYAI5tzMm0L5Z8KS\n"
  "JepiCYc/Saxdd3FkIus+VSaqKPBF3XcwfIglkZe5AzOTpQQZuSOEvKasvf2Lm+bT\n"
  "ZgWCujDmg5Fdrjake4jZRQE2ZZirFJMc6wu4XCSnwqECgYEA5PoqcmfYs14/Mf3Z\n"
  "XDSeAKqsOB61NA/A0ylvaCJixzRUylt8IgTJ9XJ0qiPOFCEcKr3DOlehZXLCieCO\n"
  "XeHDOHRSfRn4clMAQOs3EgOWRCF1IjdS99FismwPAToY3kbrCfvqN/h+cqd1lJSL\n"
  "NhboOMOPZmH1mJ29h9ukqMac4j0CgYEA46+0V6b+fd/AbfX6oLnOQcS3cyJKyPC6\n"
  "nljzjbsx123+zgAlhqStE3wL3PjzJMepS0H4h2w4owobfOcrGhpxVbWTdQRSUqpX\n"
  "X04Q+9YjDWg7OsPTQYXN1e/NCbw25WH8jPhtrZcaHwAtqPVsh94WCmKREiXg4VrU\n"
  "5MZ6z4zWGUkCgYAeBrgePIPsMYWz9ofUUYoOqFLhIRW99/rfNeXIEApH+RLNXmXO\n"
  "yDX7m8C0tvFFLnpVGIFLW0Zs2TmtfubsZLiG5KoUgZ1U0JGN8cpM8G96C7EihYK5\n"
  "wJlisEzfalDshPw5WPGD2XArdM40Z65Br4tQNkTNtjbQho7eC+1xvGnCOQKBgCzw\n"
  "NfEC5cHkUq+hWAk3Aw2aDPctcoM8eCjet5tmsgyqChuQjdeIUxzAY/sGK787pR9U\n"
  "cwAPjRIo4YoCelBZnbrj7qmu46yrMDmAR/vcpOh1hRMxKVYKWbj67oYYXuFhOJ5+\n"
  "Pe+AHki2GUz6u6QJYmJEWAuz7DGuYsyQnBaw3mT5AoGBAK83Z0rszmsS2F2xM0J3\n"
  "3xtHxaXmvJdDQto29M2+CT0vYvTBZGPT8BOPuRzkWJzDSpoSduqZNnsrh1idWL5Q\n"
  "IVNc0eS7mhAa/9QZkkU9JyyTfbzCEvchZkMD6uyFhS9zaDFUrBQjDcx0SXeoUP9i\n"
  "hOHwm+u6yzjHKIsf0M/ATOru\n"
  "-----END PRIVATE KEY-----\n";

const std::string test_key_name="boost-wintls-test-key";
#endif
