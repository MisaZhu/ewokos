# CBMC harness for the EwokOS capability core (kernel/kernel/src/cap.c).
#
#   make check                 prove every property with CBMC
#   make check P=proof_...     prove one property
#   make smoke                 compile-only sanity check with the host cc
#                              (no CBMC needed; validates the stub env)
#
# CBMC install:  brew install cbmc     (or https://github.com/diffblue/cbmc)

CBMC   ?= cbmc
CC     ?= cc
INC     = -Istub -I../../kernel/include
SRC     = cap_proofs.c

# cap.c scans CNODE_SLOTS(64) in bounded loops; unwind one past that so the
# unwinding assertions themselves prove the bound is sufficient.
UNWIND  = --unwind 66 --unwinding-assertions

PROOFS = \
	proof_cnode_insert_bounded \
	proof_grant_only_attenuates \
	proof_grant_zero_rights_rejected \
	proof_mint_requires_root \
	proof_mint_root_slot_bounded \
	proof_frame_overflow_rejected \
	proof_frame_containment_sound \
	proof_revoke_clears_slot \
	proof_root_bypass

.PHONY: check smoke clean
check:
	@if [ -n "$(P)" ]; then \
		echo "== $(P) =="; $(CBMC) $(INC) $(UNWIND) --function $(P) $(SRC); \
	else \
		for p in $(PROOFS); do \
			echo "== $$p =="; \
			$(CBMC) $(INC) $(UNWIND) --function $$p $(SRC) || exit 1; \
		done; \
		echo "ALL PROPERTIES HOLDS"; \
	fi

smoke:
	$(CC) -std=c11 -Wall -Wextra -fsyntax-only $(INC) $(SRC)
	@echo "harness compiles cleanly"

clean:
	rm -f *.o
