#include "catch.h"
#include "../AtomicSupport.h"

TEST_CASE("Test AtomicSupport.h")
{
    unsigned nrow = 100, ncol = 500;

    SECTION("Make and Convert proposals")
    {
        AtomicSupport domain('A', nrow, ncol, 1.0, 0.5);
        REQUIRE(domain.alpha() == 1.0);
        REQUIRE(domain.lambda() == 0.5);
        
        gaps::random::setSeed(1);
        AtomicProposal prop = domain.makeProposal();
        
        REQUIRE(prop.label == 'A');
        REQUIRE(prop.type == 'B');
        REQUIRE(prop.nChanges == 1);
        REQUIRE(prop.pos2 == 0);
        REQUIRE(prop.delta2 == 0.0);
        
        domain.acceptProposal(prop);

        REQUIRE(domain.numAtoms() == 1);
        REQUIRE(domain.totalMass() == prop.delta1);

        for (unsigned i = 0; i < 10000; ++i)
        {
            prop = domain.makeProposal();

            REQUIRE(prop.label == 'A');
            bool cond1 = prop.type == 'B' && prop.nChanges == 1;
            bool cond2 = prop.type == 'D' && prop.nChanges == 1;
            bool cond3 = prop.type == 'M' && prop.nChanges == 2;
            bool cond4 = prop.type == 'E' && prop.nChanges == 2;
            bool cond = cond1 || cond2 || cond3 || cond4;

            REQUIRE(cond);
            
            MatrixChange change = domain.getMatrixChange(prop);
            
            REQUIRE(change.label == 'A');
            REQUIRE(change.nChanges == prop.nChanges);
            REQUIRE(change.row2 < nrow);
            REQUIRE(change.col2 < ncol);
            REQUIRE(change.delta1 == prop.delta1);
            REQUIRE(change.delta2 == prop.delta2);

            float oldMass = domain.totalMass();

            uint64_t nOld = domain.numAtoms();

            domain.acceptProposal(prop);

            if (prop.type == 'B')
            {
                REQUIRE(domain.numAtoms() == nOld + 1);
            }
            else if (prop.type == 'D')
            {
                REQUIRE(domain.numAtoms() == nOld - 1);
            }
            else if (prop.type == 'M')
            {
                REQUIRE(domain.numAtoms() == nOld);
            }
        
            REQUIRE(domain.totalMass() == oldMass + prop.delta1 + prop.delta2);
        }
    }

    SECTION("Proposal Distribution")
    {
        AtomicSupport domain('A', nrow, ncol, 0.01, 0.05);
        
        gaps::random::setSeed(1);

        unsigned nB = 0, nD = 0, nM = 0, nE = 0;
        for (unsigned i = 0; i < 5000; ++i)
        {
            AtomicProposal prop = domain.makeProposal();
            domain.acceptProposal(prop);

            switch(prop.type)
            {
                case 'B': nB++; break;
                case 'D': nD++; break;
                case 'M': nM++; break;
                case 'E': nE++; break;
            }
        }
        REQUIRE(domain.numAtoms() > 100);
        REQUIRE(nB > 500);
        //REQUIRE(nD > 500);
        REQUIRE(nM > 500);
        REQUIRE(nE > 500);
    }
}

#ifdef GAPS_INTERNAL_TESTS

TEST_CASE("Internal AtomicSupport Tests")
{
    unsigned nrow = 29, ncol = 53;
    AtomicSupport Adomain('A', nrow, ncol, 1.0, 0.5);
    AtomicSupport Pdomain('P', nrow, ncol, 1.0, 0.5);

    std::vector<uint64_t> aAtomPos;
    std::vector<uint64_t> pAtomPos;

    for (unsigned i = 0; i < 100; ++i)
    {
        AtomicProposal propA = Adomain.proposeBirth();
        AtomicProposal propP = Pdomain.proposeBirth();
        Adomain.acceptProposal(propA);
        Pdomain.acceptProposal(propP);
        aAtomPos.push_back(propA.pos1);
        pAtomPos.push_back(propP.pos1);
    }

    REQUIRE(Adomain.numAtoms() == 100);
    REQUIRE(Pdomain.numAtoms() == 100);    

    SECTION("get row/col")
    {
        for (unsigned i = 0; i < 10000; ++i)
        {
            REQUIRE(Adomain.getRow(gaps::random::uniform64()) < nrow);
            REQUIRE(Adomain.getCol(gaps::random::uniform64()) < ncol);

            REQUIRE(Pdomain.getRow(gaps::random::uniform64()) < nrow);
            REQUIRE(Pdomain.getCol(gaps::random::uniform64()) < ncol);            
        }
    }

    SECTION("randomFreePosition")
    {
        for (unsigned i = 0; i < 10000; ++i)
        {
            uint64_t posA = Adomain.randomFreePosition();
            uint64_t posP = Pdomain.randomFreePosition();
            
            REQUIRE(std::find(aAtomPos.begin(), aAtomPos.end(), posA) == aAtomPos.end());
            REQUIRE(std::find(pAtomPos.begin(), pAtomPos.end(), posP) == pAtomPos.end());
        }
    }

    SECTION("randomAtomPosition")
    {
        for (unsigned i = 0; i < 10000; ++i)
        {
            uint64_t posA = Adomain.randomAtomPosition();
            uint64_t posP = Pdomain.randomAtomPosition();
            
            REQUIRE(std::find(aAtomPos.begin(), aAtomPos.end(), posA) != aAtomPos.end());
            REQUIRE(std::find(pAtomPos.begin(), pAtomPos.end(), posP) != pAtomPos.end());
        }
    }

/*    SECTION("updateAtomMass")
    {
        float oldMass = 0.0;
        uint64_t posA = 0, posP = 0;
        for (unsigned i = 0; i < 1000; ++i)
        {
            posA = Adomain.randomAtomPosition();
            oldMass = Adomain.at(posA);
            REQUIRE_NOTHROW(Adomain.updateAtomMass(posA, 0.05));
            REQUIRE(Adomain.at(posA) == oldMass + 0.05);

            posP = Pdomain.randomAtomPosition();
            oldMass = Pdomain.at(posP);
            REQUIRE_NOTHROW(Pdomain.updateAtomMass(posP, 0.05));
            REQUIRE(Pdomain.at(posP) == oldMass + 0.05);
        }
    }
*/
    SECTION("proposeBirth")
    {
        for (unsigned i = 0; i < 1000; ++i)
        {
            AtomicProposal propA = Adomain.proposeBirth();
            REQUIRE(propA.nChanges == 1);
            REQUIRE(std::find(aAtomPos.begin(), aAtomPos.end(), propA.pos1) == aAtomPos.end());
            REQUIRE(propA.delta1 > 0);

            AtomicProposal propP = Pdomain.proposeBirth();
            REQUIRE(propP.nChanges == 1);
            REQUIRE(std::find(pAtomPos.begin(), pAtomPos.end(), propP.pos1) == pAtomPos.end());
            REQUIRE(propP.delta1 > 0);
        }
    }

    SECTION("proposeDeath")
    {
        for (unsigned i = 0; i < 1000; ++i)
        {
            AtomicProposal propA = Adomain.proposeDeath();
            REQUIRE(propA.nChanges == 1);
            REQUIRE(std::find(aAtomPos.begin(), aAtomPos.end(), propA.pos1) != aAtomPos.end());
            REQUIRE(propA.delta1 < 0);

            AtomicProposal propP = Pdomain.proposeDeath();
            REQUIRE(propP.nChanges == 1);
            REQUIRE(std::find(pAtomPos.begin(), pAtomPos.end(), propP.pos1) != pAtomPos.end());
            REQUIRE(propP.delta1 < 0);
        }
    }

    SECTION("proposeMove")
    {
        for (unsigned i = 0; i < 1000; ++i)
        {
            AtomicProposal propA = Adomain.proposeMove();
            REQUIRE(propA.nChanges == 2);
            REQUIRE(std::find(aAtomPos.begin(), aAtomPos.end(), propA.pos1) != aAtomPos.end());
            REQUIRE(propA.delta1 < 0);
            REQUIRE(propA.delta1 == -propA.delta2);

            AtomicProposal propP = Pdomain.proposeMove();
            REQUIRE(propP.nChanges == 2);
            REQUIRE(std::find(pAtomPos.begin(), pAtomPos.end(), propP.pos1) != pAtomPos.end());
            REQUIRE(propP.delta1 < 0);
            REQUIRE(propP.delta1 == -propP.delta2);
        }
    }
        
    SECTION("proposeExchange")
    {
        for (unsigned i = 0; i < 1000; ++i)
        {
            AtomicProposal propA = Adomain.proposeExchange();
            REQUIRE(propA.nChanges == 2);
            REQUIRE(std::find(aAtomPos.begin(), aAtomPos.end(), propA.pos1) != aAtomPos.end());
            REQUIRE(propA.delta1 + propA.delta2 == 0.0);

            AtomicProposal propP = Pdomain.proposeExchange();
            REQUIRE(propP.nChanges == 2);
            REQUIRE(std::find(pAtomPos.begin(), pAtomPos.end(), propP.pos1) != pAtomPos.end());
            REQUIRE(propP.delta1 + propP.delta2 == 0.0);
        }
    }
}

#endif