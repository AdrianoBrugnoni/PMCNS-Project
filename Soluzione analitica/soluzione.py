import math

# esegui come: python3 ./soluzione.py

def sommatoria (val, i):
    return (val**i)/math.factorial(i)

def termine_fisso(m, rho):
    return ( (m*rho)**m ) / ( math.factorial(m)*(1-rho) )

def trova_soluzione ():

    # definizione variabili
    
    la = 10
    mu_i = 5
    m = 4
    p1 = 0.20;
    p2 = 0.30;
    p3 = 0.50;

    # definizione strutture

    S = 0
    S_i = 1
    Tq1 = 2
    Tq2 = 3
    Tq3 = 4
    Tq = 5
    Ts1 = 6
    Ts2 = 7
    Ts3 = 8
    Ts = 9
    Nq1 = 10
    Nq2 = 11
    Nq3 = 12
    Nq = 13
    Ns1 = 14
    Ns2 = 15
    Ns3 = 16
    Ns = 17
    E = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    p = [0]

    # calcolo valori
    
    mu = mu_i * m
    E[S] = 1/mu
    E[S_i] = 1/mu_i
    rho = la/mu
    rho1 = rho * p1
    rho2 = rho * p2
    rho3 = rho * p3
    la1 = la * p1
    la2 = la * p2
    la3 = la * p3

    print("Valori base:")
    print("\tla =", la)
    print("\tmu =", mu, ", mu_i =", mu_i, ", m =", m)
    print("\tE(s) =", E[S], ", E(S_i) =", E[S_i])
    print("\trho =", rho)
    print("\tp1 =", p1, ", p2 =", p2, ", p3 =", p3)
    print("\tla1 =", la1, ", la2 =", la2, ", la3 =", la3)
    print("\trho1 =", rho1, ", rho2 =", rho2, ", rho3 =", rho3)
    
    # calcolo probabilità code

    for i in range(0, m):
        p[0] += sommatoria(m*rho, i)
    p[0] += termine_fisso(m, rho)

    Pq = termine_fisso(m, rho)/p[0]

    print("Valori multiserver:")
    print("\tp(0)^-1 =", p[0])
    print("\tPq =", Pq)

    # calcolo tempi medi e globali di attesa nelle code
    
    E[Tq1] = (Pq*E[S]) / (1*(1-rho1))
    E[Tq2] = (Pq*E[S]) / ((1-rho1)*(1-rho1-rho2))
    E[Tq3] = (Pq*E[S]) / ((1-rho1-rho2)*(1-rho))
    E[Tq] = p1*E[Tq1] + p2*E[Tq2] + p3*E[Tq3]

    print("Tempi in coda:")
    print("\tE(Tq1) =", E[Tq1])
    print("\tE(Tq2) =", E[Tq2])
    print("\tE(Tq3) =", E[Tq3])
    print("\tE(Tq) =", E[Tq])
    
    # calcolo numero job medi e globali nelle code

    E[Nq1] = la1*E[Tq1]
    E[Nq2] = la2*E[Tq2]
    E[Nq3] = la3*E[Tq3]
    E[Nq] = E[Nq1] + E[Nq2] + E[Nq3] # la*E[Tq]

    print("Job in coda:")
    print("\tE(Nq1) =", E[Nq1])
    print("\tE(Nq2) =", E[Nq2])
    print("\tE(Nq3) =", E[Nq3])
    print("\tE(Nq) =", E[Nq])
    
    # calcolo tempi medi e globali di attesa nel sistema

    E[Ts1] = E[Tq1] + E[S_i] 
    E[Ts2] = E[Tq2] + E[S_i]
    E[Ts3] = E[Tq3] + E[S_i]
    E[Ts] = p1*E[Ts1] + p2*E[Ts2] + p3*E[Ts3]

    print("Tempi nel sistema:")
    print("\tE(Ts1) =", E[Ts1])
    print("\tE(Ts2) =", E[Ts2])
    print("\tE(Ts3) =", E[Ts3])
    print("\tE(Ts) =", E[Ts])

    # calcolo numero job medi e globali nel sistema
    
    E[Ns1] = E[Nq1] + m*rho1 # la1*E[Ts1]
    E[Ns2] = E[Nq2] + m*rho2 # la2*E[Ts2]
    E[Ns3] = E[Nq3] + m*rho3 # la3*E[Ts3]
    E[Ns] = E[Ns1] + E[Ns2] + E[Ns3] # E[Nq] + m*rho # la*E[Ts]

    print("Job nel sistema:")
    print("\tE(Ns1) =", E[Ns1])
    print("\tE(Ns2) =", E[Ns2])
    print("\tE(Ns3) =", E[Ns3])
    print("\tE(Ns) =", E[Ns])
    
    return


trova_soluzione ();
