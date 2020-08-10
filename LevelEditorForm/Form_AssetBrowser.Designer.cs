namespace LevelEditorForm
{
    partial class AssetBrowserForm
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AssetBrowserForm));
            this.LV_Browser = new System.Windows.Forms.ListView();
            this.LargeIconList = new System.Windows.Forms.ImageList(this.components);
            this.MN_Browser = new System.Windows.Forms.MenuStrip();
            this.newToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.fBXFileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.fBXToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.MN_Browser.SuspendLayout();
            this.SuspendLayout();
            // 
            // LV_Browser
            // 
            this.LV_Browser.Dock = System.Windows.Forms.DockStyle.Fill;
            this.LV_Browser.HideSelection = false;
            this.LV_Browser.LargeImageList = this.LargeIconList;
            this.LV_Browser.Location = new System.Drawing.Point(0, 24);
            this.LV_Browser.Margin = new System.Windows.Forms.Padding(0);
            this.LV_Browser.MultiSelect = false;
            this.LV_Browser.Name = "LV_Browser";
            this.LV_Browser.Size = new System.Drawing.Size(1164, 173);
            this.LV_Browser.TabIndex = 0;
            this.LV_Browser.UseCompatibleStateImageBehavior = false;
            // 
            // LargeIconList
            // 
            this.LargeIconList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("LargeIconList.ImageStream")));
            this.LargeIconList.TransparentColor = System.Drawing.Color.Transparent;
            this.LargeIconList.Images.SetKeyName(0, "folder.png");
            this.LargeIconList.Images.SetKeyName(1, "file.png");
            // 
            // MN_Browser
            // 
            this.MN_Browser.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.newToolStripMenuItem});
            this.MN_Browser.Location = new System.Drawing.Point(0, 0);
            this.MN_Browser.Name = "MN_Browser";
            this.MN_Browser.Size = new System.Drawing.Size(1164, 24);
            this.MN_Browser.TabIndex = 1;
            this.MN_Browser.Text = "menuStrip1";
            // 
            // newToolStripMenuItem
            // 
            this.newToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fBXFileToolStripMenuItem});
            this.newToolStripMenuItem.Name = "newToolStripMenuItem";
            this.newToolStripMenuItem.Size = new System.Drawing.Size(43, 20);
            this.newToolStripMenuItem.Text = "New";
            // 
            // fBXFileToolStripMenuItem
            // 
            this.fBXFileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fBXToolStripMenuItem});
            this.fBXFileToolStripMenuItem.Name = "fBXFileToolStripMenuItem";
            this.fBXFileToolStripMenuItem.Size = new System.Drawing.Size(180, 22);
            this.fBXFileToolStripMenuItem.Text = "Convert";
            // 
            // fBXToolStripMenuItem
            // 
            this.fBXToolStripMenuItem.Name = "fBXToolStripMenuItem";
            this.fBXToolStripMenuItem.Size = new System.Drawing.Size(180, 22);
            this.fBXToolStripMenuItem.Text = "FBX";
            this.fBXToolStripMenuItem.Click += new System.EventHandler(this.fBXToolStripMenuItem_Click);
            // 
            // AssetBrowserForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1164, 197);
            this.Controls.Add(this.LV_Browser);
            this.Controls.Add(this.MN_Browser);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
            this.MainMenuStrip = this.MN_Browser;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "AssetBrowserForm";
            this.Text = "AssetBrowser";
            this.MN_Browser.ResumeLayout(false);
            this.MN_Browser.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.ImageList LargeIconList;
        internal System.Windows.Forms.ListView LV_Browser;
        private System.Windows.Forms.MenuStrip MN_Browser;
        private System.Windows.Forms.ToolStripMenuItem newToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem fBXFileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem fBXToolStripMenuItem;
    }
}