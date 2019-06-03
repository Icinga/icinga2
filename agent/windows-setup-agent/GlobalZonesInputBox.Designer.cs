namespace Icinga
{
	partial class GlobalZonesInputBox
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing)
		{
			if (disposing && (components != null))
			{
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.btnOK = new System.Windows.Forms.Button();
			this.btnCancel = new System.Windows.Forms.Button();
			this.label1 = new System.Windows.Forms.Label();
			this.lblGlobalZoneName = new System.Windows.Forms.Label();
			this.txtGlobalZoneName = new System.Windows.Forms.TextBox();
			this.SuspendLayout();
			// 
			// btnOK
			// 
			this.btnOK.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.btnOK.Location = new System.Drawing.Point(191, 76);
			this.btnOK.Name = "btnOK";
			this.btnOK.Size = new System.Drawing.Size(75, 23);
			this.btnOK.TabIndex = 0;
			this.btnOK.Text = "OK";
			this.btnOK.UseVisualStyleBackColor = true;
			this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
			// 
			// btnCancel
			// 
			this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.btnCancel.Location = new System.Drawing.Point(272, 76);
			this.btnCancel.Name = "btnCancel";
			this.btnCancel.Size = new System.Drawing.Size(75, 23);
			this.btnCancel.TabIndex = 1;
			this.btnCancel.Text = "Cancel";
			this.btnCancel.UseVisualStyleBackColor = true;
			// 
			// label1
			// 
			this.label1.AutoSize = true;
			this.label1.Location = new System.Drawing.Point(13, 13);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(231, 13);
			this.label1.TabIndex = 2;
			this.label1.Text = "Please enter the name for the new global Zone:";
			// 
			// lblGlobalZoneName
			// 
			this.lblGlobalZoneName.AutoSize = true;
			this.lblGlobalZoneName.Location = new System.Drawing.Point(16, 46);
			this.lblGlobalZoneName.Name = "lblGlobalZoneName";
			this.lblGlobalZoneName.Size = new System.Drawing.Size(68, 13);
			this.lblGlobalZoneName.TabIndex = 3;
			this.lblGlobalZoneName.Text = "Global Zone:";
			// 
			// txtGlobalZoneName
			// 
			this.txtGlobalZoneName.Location = new System.Drawing.Point(90, 43);
			this.txtGlobalZoneName.Name = "txtGlobalZoneName";
			this.txtGlobalZoneName.Size = new System.Drawing.Size(257, 20);
			this.txtGlobalZoneName.TabIndex = 0;
			// 
			// GlobalZonesInputBox
			// 
			this.AcceptButton = this.btnOK;
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.CancelButton = this.btnCancel;
			this.ClientSize = new System.Drawing.Size(359, 111);
			this.Controls.Add(this.txtGlobalZoneName);
			this.Controls.Add(this.lblGlobalZoneName);
			this.Controls.Add(this.label1);
			this.Controls.Add(this.btnCancel);
			this.Controls.Add(this.btnOK);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "GlobalZonesInputBox";
			this.ShowIcon = false;
			this.ShowInTaskbar = false;
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Add Global Zones";
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.Button btnOK;
		private System.Windows.Forms.Button btnCancel;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.Label lblGlobalZoneName;
		public System.Windows.Forms.TextBox txtGlobalZoneName;
	}
}