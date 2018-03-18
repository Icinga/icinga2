using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Icinga
{
	public partial class GlobalZonesInputBox : Form
	{
		private ListView.ListViewItemCollection globalZonesItems;

		public GlobalZonesInputBox(ListView.ListViewItemCollection globalZonesItems)
		{
			InitializeComponent();

			this.globalZonesItems = globalZonesItems;
		}

		private void Warning(string message)
		{
			MessageBox.Show(this, message, Text, MessageBoxButtons.OK, MessageBoxIcon.Warning);
		}


		private void btnOK_Click(object sender, EventArgs e)
		{
			if (txtGlobalZoneName.Text == "global-templates" || txtGlobalZoneName.Text == "director-global") {
				Warning("This global zone is configured by default.");
				return;
			}

			foreach (ListViewItem lvw in globalZonesItems) {
				if (txtGlobalZoneName.Text == lvw.Text) {
					Warning("This global zone is already defined.");
					return;
				}
			}

			DialogResult = DialogResult.OK;
			Close();
		}
	}
}
