# SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-3.0-or-later

set(ICINGA2_OPENTELEMETRY_PROTOS_DIR "${icinga2_SOURCE_DIR}/third-party/opentelemetry-proto")
protobuf_generate(
  LANGUAGE cpp
  # According to the Protobuf docs[^1], the Protobuf compiler generates with the "LITE_RUNTIME" option much
  # smaller code than the default optimze_for=SPEED option, which includes code for reflection, descriptors,
  # and other features not needed by any part of the Icinga 2 OpenTelemetry integration. Thus, we use the "lite"
  # option to generate code that only depend on the libprotobuf-lite instead of the full libprotobuf library.
  #
  # The only downside of using the lite runtime is that we won't be able to use any debugging capabilities
  # provided by the full Protobuf runtime (like the DebugString() method on messages for easy printing,
  # which heavily relies on reflection).
  #
  # [^1]: https://protobuf.dev/programming-guides/proto3/#options
  PLUGIN_OPTIONS lite
  OUT_VAR otel_PROTO_SRCS
  IMPORT_DIRS "${ICINGA2_OPENTELEMETRY_PROTOS_DIR}"
  PROTOS
    "${ICINGA2_OPENTELEMETRY_PROTOS_DIR}/opentelemetry/proto/collector/metrics/v1/metrics_service.proto"
    "${ICINGA2_OPENTELEMETRY_PROTOS_DIR}/opentelemetry/proto/common/v1/common.proto"
    "${ICINGA2_OPENTELEMETRY_PROTOS_DIR}/opentelemetry/proto/metrics/v1/metrics.proto"
    "${ICINGA2_OPENTELEMETRY_PROTOS_DIR}/opentelemetry/proto/resource/v1/resource.proto"
)

set(otel_SOURCES
  otel.cpp otel.hpp
  ${otel_PROTO_SRCS}
)

add_library(otel OBJECT ${otel_SOURCES})
add_dependencies(otel base remote)
target_include_directories(otel
  SYSTEM PUBLIC
    ${Protobuf_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set_target_properties(
  otel PROPERTIES
  FOLDER Lib
)
