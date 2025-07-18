import { Classes, DialogBody, IconsModule, SidebarNavigation, SidebarNavigationPage, DefinePluginFn } from '@steambrew/client';
import { locale } from '../../locales';
import { settingsClasses } from '../utils/classes';
import { GeneralViewModal } from './general';
import { MillenniumIcons } from '../components/Icons';
import { ThemeViewModal } from './themes';
import { PluginViewModal } from './plugins';
import { RenderUpdatesSettingsTab, UpdatesViewModal } from './updates';
import { UpdateContextProvider } from './updates/useUpdateContext';
import { RenderLogViewer } from './logs';
import { ConfigProvider } from '../config-provider';
import Styles from '../utils/styles';
import { useQuickAccessStore } from '../quick-access/quickAccessStore';
import { getPluginRenderers } from '../utils/globals';

declare global {
	const g_PopupManager: any;
}

const tabSpotGeneral: SidebarNavigationPage = {
	visible: true,
	title: locale.settingsPanelGeneral,
	icon: <MillenniumIcons.SteamBrewLogo />,
	content: (
		<DialogBody className={Classes.SettingsDialogBodyFade}>
			<GeneralViewModal />
		</DialogBody>
	),
	route: '/millennium/settings/general',
};

const tabSpotThemes: SidebarNavigationPage = {
	visible: true,
	title: locale.settingsPanelThemes,
	icon: <MillenniumIcons.Themes />,
	content: (
		<DialogBody className={Classes.SettingsDialogBodyFade}>
			<ThemeViewModal />
		</DialogBody>
	),
	route: '/millennium/settings/themes',
};

const tabSpotPlugins: SidebarNavigationPage = {
	visible: true,
	title: locale.settingsPanelPlugins,
	icon: <MillenniumIcons.Plugins />,
	content: (
		<DialogBody className={Classes.SettingsDialogBodyFade}>
			<PluginViewModal />
		</DialogBody>
	),
	route: '/millennium/settings/plugins',
};

const tabSpotUpdates: SidebarNavigationPage = {
	visible: true,
	title: <RenderUpdatesSettingsTab />,
	icon: <IconsModule.Update />,
	content: (
		<DialogBody className={Classes.SettingsDialogBodyFade}>
			<UpdateContextProvider>
				<UpdatesViewModal />
			</UpdateContextProvider>
		</DialogBody>
	),
	route: '/millennium/settings/updates',
};

const tabSpotLogs: SidebarNavigationPage = {
	visible: true,
	title: locale.settingsPanelLogs,
	icon: <IconsModule.TextCodeBlock />,
	content: (
		<DialogBody className={Classes.SettingsDialogBodyFade}>
			<RenderLogViewer />
		</DialogBody>
	),
	route: '/millennium/settings/logs',
};

export function MillenniumSettings() {
	const className = `${settingsClasses.SettingsModal} ${settingsClasses.DesktopPopup} MillenniumSettings ModalDialogPopup`;
	const settingsPages = [tabSpotGeneral, 'separator', tabSpotThemes, tabSpotPlugins, 'separator', tabSpotUpdates, tabSpotLogs];

	return (
		<ConfigProvider>
			<Styles />
			{/* @ts-ignore */}
			<SidebarNavigation className={className} pages={settingsPages} title={<></>} />
		</ConfigProvider>
	);
}

function RenderSettingsModal(_: any, retObj: any) {
	const index = retObj.props.menuItems.findIndex((prop: any) => prop.name === '#Menu_Settings');

	if (index !== -1) {
		retObj.props.menuItems.splice(index + 1, 0, {
			name: 'Millennium',
			onClick: () => {
				useQuickAccessStore.getState().openQuickAccess();
			},
			visible: true,
		});
	}

	return retObj.type(retObj.props);
}

export { RenderSettingsModal };
