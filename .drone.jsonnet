local Build(name, imageSuffix, script, failure = "") = {
  kind: "pipeline",
  name: name,
  type: "kubernetes",
  metadata: {
    namespace: "drone"
  },
  node_selector: {
    "magnum.openstack.org/nodegroup": "build-worker"
  },
  image_pull_secrets: [
    "gitlab-docker-registry-credentials"
  ],
  failure: failure,
  steps: [
    {
      name: "build",
      resources: {
        limits: {
          cpu: 2000,
          memory: "3584MiB"
        }
      },
      image: "registry.icinga.com/build-docker/" + imageSuffix,
      commands: [
        ".drone/" + script + ".sh"
      ]
    }
  ]
};

local DebArch(distro, codename, bits, imageSuffix) = [
  Build(distro + " " + codename + " x" + bits, distro + "/" + codename + imageSuffix, "deb")
];

local Deb(distro, codename, has32bit = true) =
  DebArch(distro, codename, 64, "") +
  (if has32bit then DebArch(distro, codename, 32, ":x86") else []);

local RPM(distro, release) = Build(
  distro + " " + release,
  distro + "/" + release,
  "rpm",
  (if distro == "sles" then "ignore" else "")
);

Deb("debian", "buster") +
Deb("debian", "stretch") +
Deb("ubuntu", "focal", false) +
Deb("ubuntu", "bionic") +
Deb("ubuntu", "xenial") +
[
  RPM("centos", "8"),
  RPM("centos", "7"),
  RPM("centos", "6"),
  RPM("fedora", "32"),
  RPM("fedora", "31"),
  RPM("fedora", "30"),
  RPM("fedora", "29"),
  RPM("sles", "15.1"),
  RPM("sles", "15.0"),
  RPM("sles", "12.5"),
  RPM("sles", "12.4"),
  RPM("opensuse", "15.1"),
  RPM("opensuse", "15.0"),
]
