using System;
using System.IO;
using System.Text;
using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Threading;
using System.Net.NetworkInformation;
using Microsoft.Win32;
using System.IO.Compression;
using System.Diagnostics;
using System.ServiceProcess;
using System.Security.AccessControl;
using tar_cs;

namespace Icinga
{
	public partial class AgentWizard : Form
	{
		[DllImport("base", CallingConvention = CallingConvention.Cdecl)]
		private extern static int MakeX509CSR(string cn, string keyfile, string csrfile);

		delegate void FormCallback();

		public AgentWizard()
		{
			InitializeComponent();

			txtInstanceName.Text = Icinga2InstanceName;
		}

		private void FatalError(string message)
		{
			MessageBox.Show(this, message, Text, MessageBoxButtons.OK, MessageBoxIcon.Error);
			Application.Exit();
		}

		private void Warning(string message)
		{
			MessageBox.Show(this, message, Text, MessageBoxButtons.OK, MessageBoxIcon.Warning);
		}

		private string Icinga2InstallDir
		{
			get
			{
				RegistryKey rk = Registry.LocalMachine.OpenSubKey("SOFTWARE\\Icinga Development Team\\ICINGA2");

				if (rk == null)
					return "";

				return (string)rk.GetValue("");
			}
		}

		private string Icinga2InstanceName
		{
			get
			{
				IPGlobalProperties props = IPGlobalProperties.GetIPGlobalProperties();

				string fqdn = props.HostName;

				if (props.DomainName != "")
					fqdn += "." + props.DomainName;

				return fqdn;
			}
		}

		private void EnableFeature(string feature)
		{
			using (FileStream fp = File.Open(Icinga2InstallDir + String.Format("\\etc\\icinga2\\features-enabled\\{0}.conf", feature), FileMode.Create)) {
				using (StreamWriter sw = new StreamWriter(fp, Encoding.ASCII)) {
					sw.Write(String.Format("include \"../features-available/{0}.conf\"\n", feature));
				}
			}
		}

		private void GenerateHostKey()
		{
			if (!File.Exists(Icinga2InstallDir + "\\etc\\icinga2\\pki\\agent\\agent.key") ||
			    !File.Exists(Icinga2InstallDir + "\\etc\\icinga2\\pki\\agent\\agent.csr")) {
				try {
					MakeX509CSR(Icinga2InstanceName,
					    Icinga2InstallDir + "\\etc\\icinga2\\pki\\agent\\agent.key",
					    Icinga2InstallDir + "\\etc\\icinga2\\pki\\agent\\agent.csr");
				} catch (Exception ex) {
					FatalError("MakeX509CSR failed: " + ex.Message);
				}
			}

			FinishHostKey();
		}

		private void FinishHostKey()
		{
			if (InvokeRequired) {
				Invoke(new FormCallback(FinishHostKey));
				return;
			}

			txtCSR.Text = File.ReadAllText(Icinga2InstallDir + "\\etc\\icinga2\\pki\\agent\\agent.csr").Replace("\n", "\r\n");

			if (!File.Exists(Icinga2InstallDir + "\\etc\\icinga2\\pki\\agent\\agent.crt"))
				tbcPages.SelectedTab = tabCSR;
			else
				tbcPages.SelectedTab = tabParameters;
		}

		private void SetConfigureStatus(int pct, string message)
		{
			if (InvokeRequired) {
				Invoke(new FormCallback(() => SetConfigureStatus(pct, message)));
				return;
			}

			prgConfig.Value = pct;
			lblConfigStatus.Text = message;
		}

		private void ConfigureService()
		{
			SetConfigureStatus(0, "Updating configuration files...");
			using (FileStream fp = File.Open(Icinga2InstallDir + "\\etc\\icinga2\\features-available\\agent.conf", FileMode.Create)) {
				using (StreamWriter sw = new StreamWriter(fp, Encoding.ASCII)) {
					sw.Write(
					    "/**\n" +
					    " * The agent listener accepts checks from agents.\n" +
					    " */\n" +
					    "\n" +
					    "library \"agent\"\n" +
					    "\n" +
					    "object AgentListener \"agent\" {\n" +
					    "  cert_path = SysconfDir + \"/icinga2/pki/agent/agent.crt\"\n" +
					    "  key_path = SysconfDir + \"/icinga2/pki/agent/agent.key\"\n" +
					    "  ca_path = SysconfDir + \"/icinga2/pki/agent/ca.crt\"\n"
					);

					if (rdoNoMaster.Checked)
						sw.Write("  upstream_name = \"{0}\"\n", txtMasterInstance.Text);

					if (rdoListener.Checked)
						sw.Write("  bind_port = \"{0}\"\n", txtListenerPort.Text);

					if (rdoConnect.Checked)
						sw.Write(
						    "  upstream_host = \"{0}\"\n" +
						    "  upstream_port = \"{1}\"\n", txtPeerHost.Text, txtPeerPort.Text
						);

					sw.Write("}\n");
				}
			}

			EnableFeature("agent");
			EnableFeature("checker");

			SetConfigureStatus(50, "Setting ACLs for the Icinga 2 directory...");
			DirectoryInfo di = new DirectoryInfo(Icinga2InstallDir);
			DirectorySecurity ds = di.GetAccessControl();
			FileSystemAccessRule rule = new FileSystemAccessRule("NT AUTHORITY\\NetworkService",
			    FileSystemRights.ReadAndExecute | FileSystemRights.Write | FileSystemRights.ListDirectory,
			    InheritanceFlags.ObjectInherit | InheritanceFlags.ContainerInherit, PropagationFlags.None, AccessControlType.Allow);
			ds.AddAccessRule(rule);
			di.SetAccessControl(ds);

			SetConfigureStatus(75, "Installing the Icinga 2 service...");
			ProcessStartInfo psi = new ProcessStartInfo();
			psi.FileName = Icinga2InstallDir + "\\sbin\\icinga2.exe";
			psi.Arguments = "--scm-install -c \"" + Icinga2InstallDir + "\\etc\\icinga2\\icinga2.conf\"";
			psi.CreateNoWindow = true;
			psi.UseShellExecute = false;

			using (Process proc = Process.Start(psi)) {
				proc.WaitForExit();

				if (proc.ExitCode != 0)
					FatalError("The Windows service could not be installed.");
			}

			SetConfigureStatus(100, "Finished.");

			FinishConfigure();
		}

		private void FinishConfigure()
		{
			if (InvokeRequired) {
				Invoke(new FormCallback(FinishConfigure));
				return;
			}

			tbcPages.SelectedTab = tabFinish;
		}

		private void AgentWizard_Shown(object sender, EventArgs e)
		{
			string installDir = Icinga2InstallDir;

			if (installDir == "")
				FatalError("Icinga 2 does not seem to be installed properly.");

			/* TODO: This is something the NSIS installer should do */
			Directory.CreateDirectory(installDir + "\\var\\cache\\icinga2");
			Directory.CreateDirectory(installDir + "\\var\\lib\\icinga2\\agent\\inventory");
			Directory.CreateDirectory(installDir + "\\var\\lib\\icinga2\\cluster\\config");
			Directory.CreateDirectory(installDir + "\\var\\lib\\icinga2\\cluster\\log");
			Directory.CreateDirectory(installDir + "\\var\\log\\icinga2\\compat\\archive");
			Directory.CreateDirectory(installDir + "\\var\\run\\icinga2\\cmd");
			Directory.CreateDirectory(installDir + "\\var\\spool\\icinga2\\perfdata");
			Directory.CreateDirectory(installDir + "\\var\\spool\\icinga2\\tmp");

			Directory.CreateDirectory(installDir + "\\etc\\icinga2\\pki\\agent");

			Thread thread = new Thread(GenerateHostKey);
			thread.IsBackground = true;
			thread.Start();
		}

		private void btnBack_Click(object sender, EventArgs e)
		{
			tbcPages.SelectedIndex--;
		}

		private void btnNext_Click(object sender, EventArgs e)
		{
			if (tbcPages.SelectedTab == tabParameters) {
				if (rdoNoMaster.Checked && txtMasterInstance.Text == "") {
					Warning("You need to enter the name of the master instance.");
					return;
				}

				if (rdoConnect.Checked && (txtPeerHost.Text == "" || txtPeerPort.Text == "")) {
					Warning("You need to specify a host and port.");
					return;
				}

				if (rdoListener.Checked && (txtListenerPort.Text == "")) {
					Warning("You need to specify a listener port.");
					return;
				}

				if (rdoNoListener.Checked && rdoNoConnect.Checked) {
					Warning("You need to enable the listener or outbound connects.");
					return;
				}
			}

			if (tbcPages.SelectedTab == tabFinish)
				Application.Exit();

			tbcPages.SelectedIndex++;
			btnBack.Enabled = true;
		}

		private void btnCancel_Click(object sender, EventArgs e)
		{
			Application.Exit();
		}

		private void tbcPages_SelectedIndexChanged(object sender, EventArgs e)
		{
			Refresh();

			btnBack.Enabled = (tbcPages.SelectedTab != tabCSR && tbcPages.SelectedTab != tabFinish);
			btnNext.Enabled = true;

			if (tbcPages.SelectedTab == tabFinish) {
				btnNext.Text = "&Finish >";
				btnCancel.Enabled = false;
			}

			if (tbcPages.SelectedTab == tabParameters &&
			    !File.Exists(Icinga2InstallDir + "\\etc\\icinga2\\pki\\agent\\agent.crt")) {
				byte[] bytes = Convert.FromBase64String(txtBundle.Text);
				MemoryStream ms = new MemoryStream(bytes);
				GZipStream gz = new GZipStream(ms, CompressionMode.Decompress);
				MemoryStream ms2 = new MemoryStream();

				byte[] buffer = new byte[512];
				int rc;
				while ((rc = gz.Read(buffer, 0, buffer.Length)) > 0)
					ms2.Write(buffer, 0, rc);
				ms2.Position = 0;
				TarReader tr = new TarReader(ms2);
				tr.ReadToEnd(Icinga2InstallDir + "\\etc\\icinga2\\pki\\agent");
			}

			if (tbcPages.SelectedTab == tabConfigure) {
				Thread thread = new Thread(ConfigureService);
				thread.Start();
			}
		}

		private void RadioMaster_CheckedChanged(object sender, EventArgs e)
		{
			txtMasterInstance.Enabled = !rdoNewMaster.Checked;
		}

		private void RadioListener_CheckedChanged(object sender, EventArgs e)
		{
			txtListenerPort.Enabled = rdoListener.Checked;
		}

		private void RadioConnect_CheckedChanged(object sender, EventArgs e)
		{
			txtPeerHost.Enabled = rdoConnect.Checked;
			txtPeerPort.Enabled = rdoConnect.Checked;
		}
	}
}
