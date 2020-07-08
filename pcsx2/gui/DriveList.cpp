#include "PrecompiledHeader.h"
#include "DriveList.h"
#include "MainFrame.h"
#include "../CDVD/CDVDdiscReader.h"

DriveList::DriveList()
{
	Menu = new wxMenu();
	Manager = std::unique_ptr<DriveListManager>(new DriveListManager(Menu));
}

DriveListManager::DriveListManager(wxMenu* menu)
	: m_Menu( menu )
{
	m_Menu->Append(MenuId_DriveListRefresh, _("&Refresh"));
	m_Menu->AppendSeparator();
	RefreshList();

	// Bind on app level so that the event can be accessed outside DriveListManager
	wxGetApp().Bind(wxEVT_MENU, &DriveListManager::OnRefreshClicked, this, MenuId_DriveListRefresh);
}

void DriveListManager::ClearList()
{
	for (uint i = 0; i < m_Items.size(); i++)
	{
		m_Menu->Unbind(wxEVT_MENU, &DriveListManager::OnChangedSelection, this, m_Items.at(i)->itemPtr->GetId());
		m_Menu->Destroy(m_Items.at(i)->itemPtr);
	}

	m_Items.clear();
}

void DriveListManager::RefreshList()
{
	ClearList();
	auto drives = GetOpticalDriveList();
	bool itemChecked = false;

	for (auto i : drives)
	{
		std::unique_ptr<DriveListItem> dli = std::unique_ptr<DriveListItem>(new DriveListItem());
		dli->driveLetter = i;
		dli->itemPtr = m_Menu->AppendRadioItem(wxID_ANY, i);

		if (g_Conf->CurrentDisc == dli->driveLetter)
		{
			dli->itemPtr->Check(true);
			itemChecked = true;
		}

		m_Menu->Bind(wxEVT_MENU, &DriveListManager::OnChangedSelection, this, dli->itemPtr->GetId());
		m_Items.push_back(std::move(dli));
	}

	// Last used drive not found so use first found drive
	if (itemChecked == false && m_Items.size() > 0)
	{
		m_Items.at(0)->itemPtr->Check(true);

		g_Conf->Folders.RunDisc = wxDirName(m_Items.at(0)->driveLetter);
		AppSaveSettings();
	}
}

void DriveListManager::OnChangedSelection( wxCommandEvent& evt )
{
	uint index = m_Items.size();

	for (uint i = 0; i < m_Items.size(); i++)
	{
		if( (m_Items.at(i)->itemPtr != NULL) && (m_Items.at(i)->itemPtr->GetId() == evt.GetId()) )
		{
			index = i;
			break;
		}
	}

	if (index >= m_Items.size())
	{
		evt.Skip();
		return;
	}

	m_Items.at(index)->itemPtr->Check(true);

	ScopedCoreThreadPopup paused_core;
	SwapOrReset_Disc(m_Menu->GetWindow(), paused_core, m_Items.at(index)->driveLetter);
	g_Conf->Folders.RunDisc = wxDirName(m_Items.at(index)->driveLetter);
	AppSaveSettings();
}

void DriveListManager::OnRefreshClicked( wxCommandEvent& evt )
{
	RefreshList();
}

