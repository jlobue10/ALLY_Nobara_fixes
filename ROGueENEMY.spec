%global _name   rogue-enemy

Name:           rogue-enemy
Version:        2.0.0
Release:        1%{?dist}
Summary:        Convert ROG Ally [RC71L] input to DualShock4 or DualSense and allows mode switching with a long CC press

License:        GPL3
URL:            https://github.com/NeroReflex/ROGueENEMY
Source1:        rogue-enemy.service
Source2:        stray-ally.service
Source3:        80-playstation.rules
Source4:        80-playstation-no-libinput.rules
Source5:        99-js-block.rules
Source6:        99-xbox360-block.rules
Source7:        config.cfg
Source8:        rogue-enemy_iio_buffer_off.sh
Source9:        rogue-enemy_iio_buffer_on.sh

BuildRequires:  cmake libconfig-devel libevdev-devel libudev-devel
Requires:       libconfig libevdev
Recommends:     steam gamescope-session
Provides:       rogue-enemy
Conflicts:      rogue-enemy

%description
Convert ROG Ally [RC71L] input to DualShock4 or DualSense and allows mode switching with a long CC press

%prep
rm -rf %{_builddir}/ROGueENEMY
cd %{_builddir}
git clone %{url}
mkdir -p %{_builddir}/ROGueENEMY/build
cp %{_builddir}/ROGueENEMY/80-playstation.rules $RPM_SOURCE_DIR
cp %{_builddir}/ROGueENEMY/80-playstation-no-libinput.rules $RPM_SOURCE_DIR
cp %{_builddir}/ROGueENEMY/99-js-block.rules $RPM_SOURCE_DIR
cp %{_builddir}/ROGueENEMY/99-xbox360-block.rules $RPM_SOURCE_DIR
cp %{_builddir}/ROGueENEMY/config.cfg.default $RPM_SOURCE_DIR/config.cfg
cp %{_builddir}/ROGueENEMY/rogue-enemy_iio_buffer_off.sh $RPM_SOURCE_DIR
cp %{_builddir}/ROGueENEMY/rogue-enemy_iio_buffer_on.sh $RPM_SOURCE_DIR
cd $RPM_SOURCE_DIR
wget https://github.com/rog-ally-gaming/rogue-enemy/raw/main/rogue-enemy.service
wget https://github.com/rog-ally-gaming/rogue-enemy/raw/main/stray-ally.service

%build
cd %{_builddir}/ROGueENEMY/build
rm -f %{_builddir}/ROGueENEMY/Makefile
cmake ..
make

%install
mkdir -p %{buildroot}/usr/bin
cp %{_builddir}/ROGueENEMY/build/rogue-enemy %{buildroot}/usr/bin/rogue-enemy

mkdir -p %{buildroot}/etc/systemd/system/
mkdir -p %{buildroot}/usr/lib/udev/rules.d/
mkdir -p %{buildroot}/etc/ROGueENEMY

install -m 644 %{SOURCE1} %{buildroot}/etc/systemd/system/
install -m 644 %{SOURCE2} %{buildroot}/etc/systemd/system/
install -m 644 %{SOURCE3} %{buildroot}/usr/lib/udev/rules.d/
install -m 644 %{SOURCE4} %{buildroot}/usr/lib/udev/rules.d/
install -m 644 %{SOURCE5} %{buildroot}/usr/lib/udev/rules.d/
install -m 644 %{SOURCE6} %{buildroot}/usr/lib/udev/rules.d/
install -m 644 %{SOURCE7} %{buildroot}/etc/ROGueENEMY/config.cfg
install -m 755 %{SOURCE8} %{buildroot}/usr/bin/
install -m 755 %{SOURCE9} %{buildroot}/usr/bin/

%post
systemctl daemon-reload
systemctl enable rogue-enemy.service
systemctl start rogue-enemy.service
systemctl enable stray-ally.service
systemctl start stray-ally.service
udevadm control --reload-rules
udevadm trigger

%preun
systemctl stop rogue-enemy.service
systemctl disable rogue-enemy.service
systemctl stop stray-ally.service
systemctl disable stray-ally.service
systemctl daemon-reload

%files
/etc/systemd/system/rogue-enemy.service
/etc/systemd/system/stray-ally.service
/usr/bin/rogue-enemy
/usr/lib/udev/rules.d/80-playstation.rules
/usr/lib/udev/rules.d/80-playstation-no-libinput.rules
/usr/lib/udev/rules.d/99-js-block.rules
/usr/lib/udev/rules.d/99-xbox360-block.rules
/etc/ROGueENEMY/config.cfg
/usr/bin/rogue-enemy_iio_buffer_off.sh
/usr/bin/rogue-enemy_iio_buffer_on.sh

%changelog
* Sat Jan 6 2024 Denis Benato <dbenato.denis96@gmail.com> [2.0.0-1]
- Initial package
