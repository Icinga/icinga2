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

#include "icinga-studio/mainform.hpp"
#include "icinga-studio/aboutform.hpp"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <wx/msgdlg.h>

using namespace icinga;

MainForm::MainForm(wxWindow *parent, const Url::Ptr& url)
	: MainFormBase(parent)
{
#ifdef _WIN32
	SetIcon(wxICON(icinga));
	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
#endif /* _WIN32 */

	String port = url->GetPort();

	if (port.IsEmpty())
		port = "5665";

	m_ApiClient = new ApiClient(url->GetHost(), port, url->GetUsername(), url->GetPassword());
	m_ApiClient->GetTypes(std::bind(&MainForm::TypesCompletionHandler, this, _1, _2, true));

	std::string title = url->Format() + " - Icinga Studio";
	SetTitle(title);

	m_ObjectsList->InsertColumn(0, "Name", 0, 300);

	m_PropertyGrid->SetColumnCount(3);
}

void MainForm::TypesCompletionHandler(boost::exception_ptr eptr, const std::vector<ApiType::Ptr>& types, bool forward)
{
	if (forward) {
		CallAfter(std::bind(&MainForm::TypesCompletionHandler, this, eptr, types, false));
		return;
	}

	m_TypesTree->DeleteAllItems();

	if (eptr) {
		try {
			boost::rethrow_exception(eptr);
		} catch (const std::exception& ex) {
			std::string message = "HTTP query failed: " + std::string(ex.what());
			wxMessageBox(message, "Icinga Studio", wxOK | wxCENTRE | wxICON_ERROR, this);
			Close();
			return;
		}
	}

	wxTreeItemId rootNode = m_TypesTree->AddRoot("root");

	for (const ApiType::Ptr& type : types) {
		m_Types[type->Name] = type;
	}

	for (const ApiType::Ptr& type : types) {
		if (type->Abstract)
			continue;

		bool configObject = false;
		ApiType::Ptr currentType = type;

		for (;;) {
			if (currentType->BaseName.IsEmpty())
				break;

			currentType = m_Types[currentType->BaseName];

			if (!currentType)
				break;

			if (currentType->Name == "ConfigObject") {
				configObject = true;
				break;
			}
		}

		if (configObject) {
			std::string name = type->Name;
			m_TypesTree->AppendItem(rootNode, name, 0);
		}
	}
}

void MainForm::OnTypeSelected(wxTreeEvent& event)
{
	wxTreeItemId selectedId = m_TypesTree->GetSelection();
	wxString typeName = m_TypesTree->GetItemText(selectedId);
	ApiType::Ptr type = m_Types[typeName.ToStdString()];

	std::vector<String> attrs;
	attrs.emplace_back("__name");

	m_ApiClient->GetObjects(type->PluralName, std::bind(&MainForm::ObjectsCompletionHandler, this, _1, _2, true),
		std::vector<String>(), attrs);
}

static bool ApiObjectLessComparer(const ApiObject::Ptr& o1, const ApiObject::Ptr& o2)
{
	return o1->Name < o2->Name;
}

void MainForm::ObjectsCompletionHandler(boost::exception_ptr eptr, const std::vector<ApiObject::Ptr>& objects, bool forward)
{
	if (forward) {
		CallAfter(std::bind(&MainForm::ObjectsCompletionHandler, this, eptr, objects, false));
		return;
	}

	m_ObjectsList->DeleteAllItems();
	m_PropertyGrid->Clear();

	if (eptr) {
		try {
			boost::rethrow_exception(eptr);
		} catch (const std::exception& ex) {
			std::string message = "HTTP query failed: " + std::string(ex.what());
			wxMessageBox(message, "Icinga Studio", wxOK | wxCENTRE | wxICON_ERROR, this);
			return;
		}
	}

	std::vector<ApiObject::Ptr> sortedObjects = objects;
	std::sort(sortedObjects.begin(), sortedObjects.end(), ApiObjectLessComparer);

	for (const ApiObject::Ptr& object : sortedObjects) {
		std::string name = object->Name;
		m_ObjectsList->InsertItem(0, name);
	}
}

void MainForm::OnObjectSelected(wxListEvent& event)
{
	wxTreeItemId selectedId = m_TypesTree->GetSelection();
	wxString typeName = m_TypesTree->GetItemText(selectedId);
	ApiType::Ptr type = m_Types[typeName.ToStdString()];

	long itemIndex = -1;
	std::string objectName;

	while ((itemIndex = m_ObjectsList->GetNextItem(itemIndex,
		wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != wxNOT_FOUND) {
		objectName = m_ObjectsList->GetItemText(itemIndex);
		break;
	}

	if (objectName.empty())
		return;

	std::vector<String> names;
	names.emplace_back(objectName);

	m_ApiClient->GetObjects(type->PluralName, std::bind(&MainForm::ObjectDetailsCompletionHandler, this, _1, _2, true),
		names, std::vector<String>(), std::vector<String>(), true);
}

wxPGProperty *MainForm::ValueToProperty(const String& name, const Value& value)
{
	wxPGProperty *prop;

	if (value.IsNumber()) {
		prop = new wxFloatProperty(name.GetData(), wxPG_LABEL, value);
		prop->SetAttribute(wxPG_ATTR_UNITS, "Number");
		return prop;
	} else if (value.IsBoolean()) {
		prop = new wxBoolProperty(name.GetData(), wxPG_LABEL, value);
		prop->SetAttribute(wxPG_ATTR_UNITS, "Boolean");
		return prop;
	} else if (value.IsObjectType<Array>()) {
		wxArrayString val;
		Array::Ptr arr = value;

		{
			ObjectLock olock(arr);
			for (const Value& aitem : arr) {
				String val1 = aitem;
				val.Add(val1.GetData());
			}
		}

		prop = new wxArrayStringProperty(name.GetData(), wxPG_LABEL, val);
		prop->SetAttribute(wxPG_ATTR_UNITS, "Array");
		return prop;
	} else if (value.IsObjectType<Dictionary>()) {
		wxStringProperty *prop = new wxStringProperty(name.GetData(), wxPG_LABEL);

		Dictionary::Ptr dict = value;

		{
			ObjectLock olock(dict);
			for (const Dictionary::Pair& kv : dict) {
				if (kv.first != "type")
					prop->AppendChild(ValueToProperty(kv.first, kv.second));
			}
		}

		String type = "Dictionary";

		if (dict->Contains("type"))
			type = dict->Get("type");

		prop->SetAttribute(wxPG_ATTR_UNITS, type.GetData());

		return prop;
	} else if (value.IsEmpty() && !value.IsString()) {
		prop = new wxStringProperty(name.GetData(), wxPG_LABEL, "");
		prop->SetAttribute(wxPG_ATTR_UNITS, "Empty");
		return prop;
	} else {
		String val = value;
		prop = new wxStringProperty(name.GetData(), wxPG_LABEL, val.GetData());
		prop->SetAttribute(wxPG_ATTR_UNITS, "String");
		return prop;
	}
}

void MainForm::ObjectDetailsCompletionHandler(boost::exception_ptr eptr, const std::vector<ApiObject::Ptr>& objects, bool forward)
{
	if (forward) {
		CallAfter(std::bind(&MainForm::ObjectDetailsCompletionHandler, this, eptr, objects, false));
		return;
	}

	m_PropertyGrid->Clear();

	if (eptr) {
		try {
			boost::rethrow_exception(eptr);
		} catch (const std::exception& ex) {
			std::string message = "HTTP query failed: " + std::string(ex.what());
			wxMessageBox(message, "Icinga Studio", wxOK | wxCENTRE | wxICON_ERROR, this);
		}
	}

	wxTreeItemId selectedId = m_TypesTree->GetSelection();
	wxString typeName = m_TypesTree->GetItemText(selectedId);
	ApiType::Ptr type = m_Types[typeName.ToStdString()];

	String nameAttr = type->Name.ToLower() + ".__name";

	if (objects.empty())
		return;

	ApiObject::Ptr object = objects[0];

	std::map<String, wxStringProperty *> parents;

	for (const auto& kv : object->Attrs) {
		std::vector<String> tokens;
		boost::algorithm::split(tokens, kv.first, boost::is_any_of("."));

		std::map<String, wxStringProperty *>::const_iterator it = parents.find(tokens[0]);

		wxStringProperty *parent;

		if (it == parents.end()) {
			parent = new wxStringProperty(tokens[0].GetData(), wxPG_LABEL);
			parent->SetAttribute(wxPG_ATTR_UNITS, "Object");
			parents[tokens[0]] = parent;
		} else
			parent = it->second;

		wxPGProperty *prop = ValueToProperty(tokens[1], kv.second);
		parent->AppendChild(prop);
	}

	/* Make sure the property node for the real object (as opposed to joined objects) is the first one */
	String propName = type->Name.ToLower();
	wxStringProperty *objProp = parents[propName];

	if (objProp) {
		m_PropertyGrid->Append(objProp);
		m_PropertyGrid->SetPropertyReadOnly(objProp);
		parents.erase(propName);
	}

	for (const auto& kv : parents) {
		m_PropertyGrid->Append(kv.second);
		m_PropertyGrid->SetPropertyReadOnly(kv.second);
	}

	m_PropertyGrid->FitColumns();
}

void MainForm::OnQuitClicked(wxCommandEvent& event)
{
	Close();
}

void MainForm::OnAboutClicked(wxCommandEvent& event)
{
	AboutForm form(this);
	form.ShowModal();
}
