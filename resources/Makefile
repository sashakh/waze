PREFIX := /usr/local

SHARE_DIR:= $(PREFIX)/share/waze
USER_DIR:= $(HOME)/.waze

INSTALL := install

base_dir:= freemap

install: install-share

install-user:
	$(INSTALL) -m 0700 -d $(USER_DIR)
	$(INSTALL) -D -m 0644 $(base_dir)/font*.ttf $(USER_DIR)
	$(INSTALL) -D -m 0644 $(base_dir)/*.menu $(USER_DIR)
	$(INSTALL) -D -m 0644 $(base_dir)/sprites $(USER_DIR)
	$(INSTALL) -D -m 0644 $(base_dir)/preferences $(USER_DIR)
	$(INSTALL) -D -m 0644 $(base_dir)/prompt_list.txt $(USER_DIR)
	$(INSTALL) -d $(USER_DIR)/maps
	$(INSTALL) -D -m 0644 $(base_dir)/maps/* $(USER_DIR)/maps
	#$(INSTALL) -D -m 0644 $(base_dir)/* $(USER_DIR)

install-share:
	for f in `find $(base_dir) -type f | sed -e 's/^$(base_dir)\///'` ; do \
		$(INSTALL) -D -m 0644 $(base_dir)/$$f $(SHARE_DIR)/$$f ; \
	done

# for future use (maybe)
install-skins:
	$(MAKE) component=skins install-component

install-sound:
	$(MAKE) component=sound install-component

install-component::
	for f in `find $(base_dir)/$(component) -type f | sed -e 's/.*\/$(component)\///'` ; do \
		$(INSTALL) -D -m 0644 $(base_dir)/$(component)/$$f $(PREFIX)/share/waze/$(component)/$$f ; \
	done
