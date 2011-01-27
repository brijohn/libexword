
if DOX_ENABLED
all-local: Doxygen
endif

Doxygen:
	@(echo "Creating Documentation"; \
	  $(DOX_PROG) doxy.cfg)

clean-local:
	-rm -rf html
