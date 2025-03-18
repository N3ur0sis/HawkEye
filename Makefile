CC = gcc
CFLAGS = -Wall -Wextra -O2
SRCDIR = src
BINDIR = bin

ORCHESTRATEUR_SRC = $(SRCDIR)/orchestrateur/orchestrateur.c
AGENT_NMAP_SRC = $(SRCDIR)/agents/agent-nmap.c
AGENT_NIKTO_SRC = $(SRCDIR)/agents/agent-nikto.c
AGENT_ZAP_SRC = $(SRCDIR)/agents/agent-zap.c

ORCHESTRATEUR_BIN = $(BINDIR)/orchestrateur
AGENT_NMAP_BIN = $(BINDIR)/agent-nmap
AGENT_NIKTO_BIN = $(BINDIR)/agent-nikto
AGENT_ZAP_BIN = $(BINDIR)/agent-zap

all: $(BINDIR) $(ORCHESTRATEUR_BIN) $(AGENT_NMAP_BIN) $(AGENT_NIKTO_BIN) $(AGENT_ZAP_BIN)

$(BINDIR):
	mkdir -p $(BINDIR)

$(ORCHESTRATEUR_BIN): $(ORCHESTRATEUR_SRC)
	$(CC) $(CFLAGS) -o $@ $< -lpthread

$(AGENT_NMAP_BIN): $(AGENT_NMAP_SRC)
	$(CC) $(CFLAGS) -o $@ $<

$(AGENT_NIKTO_BIN): $(AGENT_NIKTO_SRC)
	$(CC) $(CFLAGS) -o $@ $<

$(AGENT_ZAP_BIN): $(AGENT_ZAP_SRC)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(BINDIR)

.PHONY: all clean