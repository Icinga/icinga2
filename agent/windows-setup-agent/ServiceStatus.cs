using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.ServiceProcess;
using System.Diagnostics;

namespace Icinga
{
	public partial class ServiceStatus : Form
	{
		public ServiceStatus()
		{
			InitializeComponent();

			try {
				ServiceController sc = new ServiceController("icinga2");

				txtStatus.Text = sc.Status.ToString();
			} catch (InvalidOperationException) {
				txtStatus.Text = "Not Available";
			}
		}

		private void btnReconfigure_Click(object sender, EventArgs e)
		{
			new SetupWizard().ShowDialog(this);
		}

		private void btnOK_Click(object sender, EventArgs e)
		{
			Close();
		}

		private void btnOpenConfigDir_Click(object sender, EventArgs e) {
			Process.Start("explorer.exe", Program.Icinga2DataDir);
		}
	}
}
