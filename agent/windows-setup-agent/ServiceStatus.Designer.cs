namespace Icinga
{
	partial class ServiceStatus
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ServiceStatus));
            this.picBanner = new System.Windows.Forms.PictureBox();
            this.lblStatus = new System.Windows.Forms.Label();
            this.txtStatus = new System.Windows.Forms.TextBox();
            this.btnReconfigure = new System.Windows.Forms.Button();
            this.btnOK = new System.Windows.Forms.Button();
            this.btnOpenConfigDir = new System.Windows.Forms.Button();
            ((System.ComponentModel.ISupportInitialize)(this.picBanner)).BeginInit();
            this.SuspendLayout();
            // 
            // picBanner
            // 
            this.picBanner.Image = global::Icinga.Properties.Resources.icinga_banner;
            this.picBanner.Location = new System.Drawing.Point(0, 0);
            this.picBanner.Name = "picBanner";
            this.picBanner.Size = new System.Drawing.Size(625, 77);
            this.picBanner.TabIndex = 2;
            this.picBanner.TabStop = false;
            // 
            // lblStatus
            // 
            this.lblStatus.AutoSize = true;
            this.lblStatus.Location = new System.Drawing.Point(12, 105);
            this.lblStatus.Name = "lblStatus";
            this.lblStatus.Size = new System.Drawing.Size(79, 13);
            this.lblStatus.TabIndex = 3;
            this.lblStatus.Text = "Service Status:";
            // 
            // txtStatus
            // 
            this.txtStatus.Location = new System.Drawing.Point(97, 102);
            this.txtStatus.Name = "txtStatus";
            this.txtStatus.ReadOnly = true;
            this.txtStatus.Size = new System.Drawing.Size(278, 20);
            this.txtStatus.TabIndex = 3;
            // 
            // btnReconfigure
            // 
            this.btnReconfigure.Location = new System.Drawing.Point(195, 143);
            this.btnReconfigure.Name = "btnReconfigure";
            this.btnReconfigure.Size = new System.Drawing.Size(89, 23);
            this.btnReconfigure.TabIndex = 1;
            this.btnReconfigure.Text = "Reconfigure";
            this.btnReconfigure.UseVisualStyleBackColor = true;
            this.btnReconfigure.Click += new System.EventHandler(this.btnReconfigure_Click);
            // 
            // btnOK
            // 
            this.btnOK.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnOK.Location = new System.Drawing.Point(290, 143);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(89, 23);
            this.btnOK.TabIndex = 0;
            this.btnOK.Text = "OK";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            // 
            // btnOpenConfigDir
            // 
            this.btnOpenConfigDir.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnOpenConfigDir.Location = new System.Drawing.Point(100, 143);
            this.btnOpenConfigDir.Name = "btnOpenConfigDir";
            this.btnOpenConfigDir.Size = new System.Drawing.Size(89, 23);
            this.btnOpenConfigDir.TabIndex = 2;
            this.btnOpenConfigDir.Text = "Examine Config";
            this.btnOpenConfigDir.UseVisualStyleBackColor = true;
            this.btnOpenConfigDir.Click += new System.EventHandler(this.btnOpenConfigDir_Click);
            // 
            // ServiceStatus
            // 
            this.AcceptButton = this.btnOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnOK;
            this.ClientSize = new System.Drawing.Size(391, 186);
            this.Controls.Add(this.btnOpenConfigDir);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.btnReconfigure);
            this.Controls.Add(this.txtStatus);
            this.Controls.Add(this.lblStatus);
            this.Controls.Add(this.picBanner);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.Name = "ServiceStatus";
            this.Text = "Icinga 2 Service Status";
            ((System.ComponentModel.ISupportInitialize)(this.picBanner)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.PictureBox picBanner;
		private System.Windows.Forms.Label lblStatus;
		private System.Windows.Forms.TextBox txtStatus;
		private System.Windows.Forms.Button btnReconfigure;
		private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.Button btnOpenConfigDir;
    }
}