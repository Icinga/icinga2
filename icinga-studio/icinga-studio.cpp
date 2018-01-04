/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "icinga-studio/connectform.hpp"
#include "icinga-studio/mainform.hpp"
#include "base/application.hpp"
#include <wx/wx.h>
#include <wx/app.h>
#include <wx/config.h>

using namespace icinga;

class IcingaStudio final : public wxApp
{
public:
	virtual bool OnInit() override
	{
		Application::InitializeBase();

		Url::Ptr pUrl;

		if (argc < 2) {
			wxConfig config("IcingaStudio");
			wxString wUrl;

			if (!config.Read("url", &wUrl))
				wUrl = "https://localhost:5665/";

			std::string url = wUrl.ToStdString();

			ConnectForm f(nullptr, new Url(url));
			if (f.ShowModal() != wxID_OK)
				return false;

			pUrl = f.GetUrl();
			url = pUrl->Format(false, true);
			wUrl = url;
			config.Write("url", wUrl);
		} else {
			pUrl = new Url(argv[1].ToStdString());
		}

		MainForm *m = new MainForm(nullptr, pUrl);
		m->Show();

		return true;
	}
};

wxIMPLEMENT_APP(IcingaStudio);
