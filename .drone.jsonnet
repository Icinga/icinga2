local pull = {
  pull: "always"
};

local cacheStep = pull + {
  failure: "ignore",
  image: "plugins/s3-cache:1.4"
};

local Build(name, steps, failure, cacheUrl, buildOpts = {}) =
  local cachePath = "icinga2/" + name;
  local cacheEnv = {
    PLUGIN_PULL: "true",
    PLUGIN_ENDPOINT: cacheUrl,
    PLUGIN_ACCESS_KEY: {
      from_secret: "minio-access-key"
    },
    PLUGIN_SECRET_KEY: {
      from_secret: "minio-secret-key"
    },
    PLUGIN_ROOT: "cache",
    PLUGIN_PATH: cachePath
  };
  {
    kind: "pipeline",
    name: name,
    image_pull_secrets: [
      "gitlab-docker-registry-credentials"
    ],
    failure: failure,
    steps: [
      cacheStep + {
        name: "restore cache",
        environment: cacheEnv + {
          PLUGIN_RESTORE: "true"
        }
      }
    ] + [
      pull + buildOpts + {
        name: step[0],
        image: "registry.icinga.com/build-docker/" + step[1],
        failure: step[3],
        commands: [
          ".drone/" + step[2] + ".sh"
        ]
      } for step in steps
    ] + [
      cacheStep + {
        name: "save cache",
        environment: cacheEnv + {
          PLUGIN_REBUILD: "true",
          PLUGIN_MOUNT: "ccache"
        }
      },
      cacheStep + {
        name: "cleanup cache",
        environment: cacheEnv + {
          PLUGIN_FLUSH: "true",
          PLUGIN_FLUSH_PATH: cachePath
        }
      }
    ]
  };

local K8s(name, steps, failure = "") =
  Build(name, steps, failure, "http://drone-minio:9000", {
    resources: {
      limits: {
        cpu: 1000,
        memory: "3584MiB"
      }
    }
  }) + {
    type: "kubernetes",
    metadata: {
      namespace: "drone"
    },
    node_selector: {
      "magnum.openstack.org/nodegroup": "build-worker-xxlarge"
    }
  };

local Deb(distro, codename, has32bit = true) = K8s(
  distro + " " + codename,
  [
    ["source", distro + "/" + codename, "deb/source", ""],
    ["binary x64", distro + "/" + codename, "deb/binary", ""],
    ["test x64", distro + "/" + codename, "deb/test", ""]
  ] + (if has32bit then [
    ["binary x32", distro + "/" + codename + ":x86", "deb/binary", ""],
    ["test x32", distro + "/" + codename + ":x86", "deb/test", ""]
  ] else [])
);

local RPM(distro, release) = K8s(
  distro + " " + release,
  [
    ["package", distro + "/" + release, "rpm/package", ""],
    ["test", distro + "/" + release, "rpm/test", ""]
  ],
  (if distro == "sles" then "ignore" else "")
);

local RasPi(codename) =
  Build(
    "raspbian " + codename,
    [
      ["package", "raspbian/" + codename, "raspi/package", ""],
      ["test", "raspbian/" + codename, "raspi/test", "ignore" /*
        Setting up icinga2-bin (2.12.0+rc1.25.g5d1c82a3d.20200526.0754+buster-0) ...
        enabling default icinga2 features
        qemu:handle_cpu_signal received signal outside vCPU context @ pc=0x6015c75c
        qemu:handle_cpu_signal received signal outside vCPU context @ pc=0x6015c75c
        qemu:handle_cpu_signal received signal outside vCPU context @ pc=0x600016ea
        dpkg: error processing package icinga2-bin (--configure):
         installed icinga2-bin package post-installation script subprocess returned error exit status 127
      */ ]
    ],
    "",
    "https://minio.drone.icinga.com"
  ) + {
    type: "docker",
    platform: {
      os: "linux"
    }
  };

[
  Deb("debian", "buster"),
  Deb("debian", "stretch"),
  Deb("ubuntu", "groovy", false),
  Deb("ubuntu", "focal", false),
  Deb("ubuntu", "bionic"),
  Deb("ubuntu", "xenial"),
  RPM("centos", "8"),
  RPM("centos", "7"),
  RPM("fedora", "32"),
  RPM("sles", "15.1"),
  RPM("sles", "12.5"),
  RPM("opensuse", "15.1"),
  RasPi("buster"),
]
